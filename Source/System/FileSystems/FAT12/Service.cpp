/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/FileSystems/FAT12/Service.cpp
 * FAT12 file system service.
 */

#include <ABI/Console.hpp>
#include <ABI/FileSystem.hpp>
#include <ABI/IPC.hpp>
#include <ABI/Task.hpp>

#include "Service.hpp"
#include "Tests.hpp"
#include "Volume.hpp"

#define TEST // TODO #18

namespace Quantum::System::FileSystems::FAT12 {
  using Console = ABI::Console;
  using FileSystem = ABI::FileSystem;
  using IPC = ABI::IPC;
  using Task = ABI::Task;

  void Service::InitializeVolume() {
    // ensure helpers are wired in freestanding runtime
    _volumeStorage.Init();
    _volume = &_volumeStorage;
    _volume->Load();
  }

  bool Service::IsRootPath(CString path) {
    if (!path || path[0] == '\0') {
      return true;
    }

    if (path[0] == '/' && path[1] == '\0') {
      return true;
    }

    if (path[0] == '\\' && path[1] == '\0') {
      return true;
    }

    if (path[0] == '.' && path[1] == '\0') {
      return true;
    }

    return false;
  }

  FileSystem::Handle Service::AllocateHandle(
    bool isDirectory,
    bool isRoot,
    UInt32 startCluster,
    UInt32 fileSize
  ) {
    for (UInt32 i = 0; i < _maxHandles; ++i) {
      if (!_handles[i].inUse) {
        _handles[i].inUse = true;
        _handles[i].isDirectory = isDirectory;
        _handles[i].isRoot = isRoot;
        _handles[i].startCluster = startCluster;
        _handles[i].nextIndex = 0;
        _handles[i].fileSize = fileSize;
        _handles[i].fileOffset = 0;
        _handles[i].attributes = isDirectory ? 0x10 : 0x20;
        _handles[i].entryLBA = 0;
        _handles[i].entryOffset = 0;

        return static_cast<FileSystem::Handle>(_handleBase + i);
      }
    }

    return 0;
  }

  void Service::ReleaseHandle(FileSystem::Handle handle) {
    HandleState* state = GetHandleState(handle);

    if (!state) {
      return;
    }

    state->inUse = false;
    state->isDirectory = false;
    state->isRoot = false;
    state->startCluster = 0;
    state->nextIndex = 0;
    state->fileSize = 0;
    state->fileOffset = 0;
    state->attributes = 0;
    state->entryLBA = 0;
    state->entryOffset = 0;
  }

  Service::HandleState* Service::GetHandleState(FileSystem::Handle handle) {
    if (handle < _handleBase) {
      return nullptr;
    }

    UInt32 index = handle - _handleBase;

    if (index >= _maxHandles) {
      return nullptr;
    }

    return &_handles[index];
  }

  static void NormalizeSegment(char* segment) {
    if (!segment || segment[0] == '\0') {
      return;
    }

    UInt32 out = 0;
    bool lastWasSlash = false;

    for (UInt32 i = 0; segment[i] != '\0'; ++i) {
      char c = segment[i];

      if (c == '\\') {
        c = '/';
      }

      if (c == '/') {
        if (lastWasSlash) {
          continue;
        }

        lastWasSlash = true;
        segment[out++] = '/';

        continue;
      }

      lastWasSlash = false;
      segment[out++] = c;
    }

    if (out > 1 && segment[out - 1] == '/') {
      --out;
    }

    segment[out] = '\0';
  }

  bool Service::ResolveParent(
    CString path,
    UInt32& parentCluster,
    bool& parentIsRoot,
    char* name,
    UInt32 nameBytes
  ) {
    if (!path || !name || nameBytes == 0) {
      return false;
    }

    parentCluster = 0;
    parentIsRoot = true;
    name[0] = '\0';

    char normalized[FileSystem::maxDirectoryLength] = {};
    UInt32 p = 0;

    while (path[p] != '\0' && p + 1 < sizeof(normalized)) {
      normalized[p] = path[p];
      ++p;
    }

    normalized[p] = '\0';

    NormalizeSegment(normalized);

    const char* cursor = normalized;

    while (*cursor == '/' || *cursor == '\\') {
      ++cursor;
    }

    const char* lastSegment = cursor;

    while (*cursor != '\0') {
      if (*cursor == '/') {
        lastSegment = cursor + 1;
      }

      ++cursor;
    }

    if (!lastSegment || *lastSegment == '\0') {
      return false;
    }

    UInt32 i = 0;
    const char* nameCursor = lastSegment;

    while (
      *nameCursor != '\0' &&
      *nameCursor != '/' &&
      i + 1 < nameBytes
    ) {
      name[i++] = *nameCursor++;
    }

    name[i] = '\0';
    cursor = path;

    while (*cursor == '/') {
      ++cursor;
    }

    while (*cursor != '\0' && cursor < lastSegment) {
      char segment[FileSystem::maxDirectoryLength] = {};
      UInt32 length = 0;

      while (
        *cursor != '\0' &&
        *cursor != '/' &&
        length + 1 < sizeof(segment)
      ) {
        segment[length++] = *cursor++;
      }

      segment[length] = '\0';

      while (*cursor == '/') {
        ++cursor;
      }

      if (segment[0] == '\0') {
        continue;
      }

      if (segment[0] == '.' && segment[1] == '\0') {
        continue;
      }

      if (segment[0] == '.' && segment[1] == '.' && segment[2] == '\0') {
        if (parentIsRoot) {
          continue;
        }

        UInt32 upCluster = 0;
        UInt8 upAttributes = 0;
        UInt32 upSize = 0;

        if (
          !_volume->FindEntry(
            parentCluster,
            false,
            "..",
            upCluster,
            upAttributes,
            upSize
          )
        ) {
          return false;
        }

        parentCluster = upCluster;
        parentIsRoot = (upCluster == 0);

        continue;
      }

      UInt32 nextCluster = 0;
      UInt8 attributes = 0;
      UInt32 sizeBytes = 0;

      if (
        !_volume->FindEntry(
          parentCluster,
          parentIsRoot,
          segment,
          nextCluster,
          attributes,
          sizeBytes
        )
      ) {
        return false;
      }

      if ((attributes & 0x10) == 0) {
        return false;
      }

      parentCluster = nextCluster;
      parentIsRoot = false;
    }

    return name[0] != '\0';
  }

  void Service::Main() {
    UInt32 portId = IPC::CreatePort();

    if (portId == 0) {
      Console::WriteLine("FAT12: failed to create IPC port");
      Task::Exit(1);
    }

    UInt32 reg = FileSystem::RegisterService(
      FileSystem::Type::FAT12,
      portId
    );

    if (reg != 0) {
      Console::WriteLine("FAT12: failed to register service");
      Task::Exit(1);
    }

    Console::WriteLine("FAT12 service ready");
    InitializeVolume();

    #if defined(TEST)
    Tests::Run();
    #endif

    for (;;) {
      IPC::Message msg {};

      if (IPC::Receive(portId, msg) != 0) {
        Task::Yield();

        continue;
      }

      if (msg.length < FileSystem::messageHeaderBytes) {
        continue;
      }

      FileSystem::ServiceMessage request {};

      UInt32 copyBytes = msg.length;

      if (copyBytes > sizeof(request)) {
        copyBytes = sizeof(request);
      }

      for (UInt32 i = 0; i < copyBytes; ++i) {
        reinterpret_cast<UInt8*>(&request)[i] = msg.payload[i];
      }

      if (request.replyPortId == 0) {
        continue;
      }

      FileSystem::ServiceMessage response {};

      response.op = request.op;
      response.status = 1;
      response.replyPortId = request.replyPortId;
      response.arg0 = 0;
      response.arg1 = 0;
      response.arg2 = 0;
      response.dataLength = 0;

      if (
        request.op == static_cast<UInt32>(
          FileSystem::Operation::ListVolumes
        )
      ) {
        UInt32 maxEntries = request.arg1;
        UInt32 count = (maxEntries > 0 && _volume && _volume->IsValid())
          ? 1
          : 0;

        if (
          count > 0 &&
          sizeof(FileSystem::VolumeEntry) <= FileSystem::messageDataBytes
        ) {
          // return a single "A" volume for now
          FileSystem::VolumeEntry entry {};

          _volume->FillEntry(entry);

          UInt32 bytes = static_cast<UInt32>(sizeof(FileSystem::VolumeEntry));

          for (UInt32 i = 0; i < bytes; ++i) {
            response.data[i] = reinterpret_cast<UInt8*>(&entry)[i];
          }

          response.dataLength = bytes;
          response.status = count;
        } else {
          response.status = 0;
        }
      } else if (
        request.op == static_cast<UInt32>(
          FileSystem::Operation::OpenVolume
        )
      ) {
        CString label = reinterpret_cast<CString>(request.data);

        // accept "A" or "A:" and return the fixed handle
        if (_volume && _volume->MatchesLabel(label)) {
          response.status = _volume->GetHandle();
        } else {
          response.status = 0;
        }
      } else if (
        request.op == static_cast<UInt32>(
          FileSystem::Operation::GetVolumeInfo
        )
      ) {
        if (_volume && request.arg0 == _volume->GetHandle()) {
          FileSystem::VolumeInfo info = _volume->GetInfo();
          UInt32 bytes = static_cast<UInt32>(sizeof(FileSystem::VolumeInfo));

          if (bytes <= FileSystem::messageDataBytes) {
            for (UInt32 i = 0; i < bytes; ++i) {
              response.data[i] = reinterpret_cast<UInt8*>(&info)[i];
            }

            response.dataLength = bytes;
            response.status = 0;
          }
        }
      } else if (
        request.op == static_cast<UInt32>(FileSystem::Operation::Open)
      ) {
        CString path = reinterpret_cast<CString>(request.data);

        if (_volume && request.arg0 == _volume->GetHandle()) {
          if (IsRootPath(path)) {
            FileSystem::Handle handle = AllocateHandle(true, true, 0, 0);

            response.status = handle;
          } else {
            UInt32 cluster = 0;
            UInt32 lastCluster = 0;
            UInt8 lastAttributes = 0;
            UInt32 lastSize = 0;
            const char* cursor = nullptr;
            bool isRoot = true;
            bool ok = true;
            bool sawSegment = false;
            bool endsWithSeparator = false;
            UInt32 lastParentCluster = 0;
            bool lastParentIsRoot = true;
            char lastName[FileSystem::maxDirectoryLength] = {};
            char normalized[FileSystem::maxDirectoryLength] = {};

            if (path) {
              UInt32 len = 0;
              UInt32 copy = 0;

              while (path[len] != '\0') {
                ++len;
              }

              if (len > 0) {
                char last = path[len - 1];

                endsWithSeparator = (last == '/' || last == '\\');
              }

              while (path[copy] != '\0' && copy + 1 < sizeof(normalized)) {
                normalized[copy] = path[copy];
                ++copy;
              }

              normalized[copy] = '\0';

              NormalizeSegment(normalized);
            }

            cursor = normalized;

            while (*cursor == '/') {
              ++cursor;
            }

            while (*cursor != '\0') {
              char segment[FileSystem::maxDirectoryLength] = {};
              UInt32 length = 0;

              while (
                *cursor != '\0' &&
                *cursor != '/' &&
                length + 1 < sizeof(segment)
              ) {
                segment[length++] = *cursor++;
              }

              segment[length] = '\0';
              sawSegment = true;

              bool hadSeparator = false;

              while (*cursor == '/') {
                ++cursor;
                hadSeparator = true;
              }

              if (segment[0] == '\0') {
                continue;
              }

              if (segment[0] == '.' && segment[1] == '\0') {
                if (*cursor == '\0') {
                  lastCluster = cluster;
                  lastAttributes = 0x10;
                  lastSize = 0;
                  lastParentCluster = cluster;
                  lastParentIsRoot = isRoot;
                  lastName[0] = '.';
                  lastName[1] = '\0';
                }

                continue;
              }

              if (segment[0] == '.' && segment[1] == '.' && segment[2] == '\0') {
                if (isRoot) {
                  if (*cursor == '\0') {
                    lastCluster = 0;
                    lastAttributes = 0x10;
                    lastSize = 0;
                    lastParentCluster = 0;
                    lastParentIsRoot = true;
                    lastName[0] = '.';
                    lastName[1] = '.';
                    lastName[2] = '\0';
                  }

                  continue;
                }

                UInt32 upCluster = 0;
                UInt8 upAttributes = 0;
                UInt32 upSize = 0;

                if (
                  !_volume->FindEntry(
                    cluster,
                    false,
                    "..",
                    upCluster,
                    upAttributes,
                    upSize
                  )
                ) {
                  ok = false;

                  break;
                }

                cluster = upCluster;
                isRoot = (upCluster == 0);

                if (*cursor == '\0') {
                  lastCluster = upCluster;
                  lastAttributes = 0x10;
                  lastSize = 0;
                  lastParentCluster = upCluster;
                  lastParentIsRoot = isRoot;
                  lastName[0] = '.';
                  lastName[1] = '.';
                  lastName[2] = '\0';
                }

                continue;
              }

              UInt32 nextCluster = 0;
              UInt8 attributes = 0;
              UInt32 sizeBytes = 0;
              bool lastSegment = (*cursor == '\0');
              bool requireDirectory = lastSegment && hadSeparator;

              lastParentCluster = cluster;
              lastParentIsRoot = isRoot;

              if (
                !_volume->FindEntry(
                  cluster,
                  isRoot,
                  segment,
                  nextCluster,
                  attributes,
                  sizeBytes
                )
              ) {
                ok = false;

                break;
              }

              for (UInt32 i = 0; i < sizeof(lastName); ++i) {
                lastName[i] = segment[i];

                if (segment[i] == '\0') {
                  break;
                }
              }

              lastCluster = nextCluster;
              lastAttributes = attributes;
              lastSize = sizeBytes;

              if (!lastSegment || requireDirectory) {
                if ((attributes & 0x10) == 0) {
                  ok = false;

                  break;
                }
              }

              if ((attributes & 0x10) != 0) {
                cluster = nextCluster;
                isRoot = false;
              }
            }

            if (ok && sawSegment) {
              bool isDirectory = (lastAttributes & 0x10) != 0;
              UInt32 entryLBA = 0;
              UInt32 entryOffset = 0;
              bool haveLocation = false;

              if (
                !(lastName[0] == '.' && lastName[1] == '\0') &&
                !(lastName[0] == '.' && lastName[1] == '.' && lastName[2] == '\0')
              ) {
                haveLocation = _volume->GetEntryLocation(
                  lastParentCluster,
                  lastParentIsRoot,
                  lastName,
                  entryLBA,
                  entryOffset
                );
              }

              if (endsWithSeparator && !isDirectory) {
                ok = false;
              } else if (isDirectory) {
                FileSystem::Handle handle = AllocateHandle(
                  true,
                  false,
                  lastCluster,
                  0
                );

                if (handle != 0) {
                  HandleState* state = GetHandleState(handle);

                  if (state) {
                    state->attributes = lastAttributes;
                    if (haveLocation) {
                      state->entryLBA = entryLBA;
                      state->entryOffset = entryOffset;
                    }
                  }
                }

                response.status = handle;
              } else if (lastCluster != 0 || lastSize == 0) {
                FileSystem::Handle handle = AllocateHandle(
                  false,
                  false,
                  lastCluster,
                  lastSize
                );

                if (handle != 0) {
                  HandleState* state = GetHandleState(handle);

                  if (state) {
                    state->attributes = lastAttributes;
                    state->fileSize = lastSize;

                    if (haveLocation) {
                      state->entryLBA = entryLBA;
                      state->entryOffset = entryOffset;
                    }
                  }
                }

                response.status = handle;
              }
            }
          }
        }
      } else if (
        request.op == static_cast<UInt32>(FileSystem::Operation::Close)
      ) {
        HandleState* state = GetHandleState(request.arg0);

        if (!state || !state->inUse) {
          response.status = 1;
        } else {
          ReleaseHandle(request.arg0);

          response.status = 0;
        }
      } else if (
        request.op == static_cast<UInt32>(
          FileSystem::Operation::ReadDirectory
        )
      ) {
        HandleState* state = GetHandleState(request.arg0);

        if (!_volume || !state || !state->inUse || !state->isDirectory) {
          response.status = 1;
        } else {
          FileSystem::DirectoryEntry entry {};

          bool found = false;

          if (state->isRoot) {
            while (state->nextIndex < _volume->GetRootEntryCount()) {
              bool end = false;

              if (
                _volume->ReadRootEntry(
                  state->nextIndex,
                  entry,
                  end
                )
              ) {
                found = true;
                state->nextIndex++;

                break;
              }

              state->nextIndex++;

              if (end) {
                break;
              }
            }
          } else {
            for (;;) {
              bool end = false;

              if (
                _volume->ReadDirectoryEntry(
                  state->startCluster,
                  state->nextIndex,
                  entry,
                  end
                )
              ) {
                found = true;
                state->nextIndex++;

                break;
              }

              state->nextIndex++;

              if (end) {
                break;
              }
            }
          }

          if (found) {
            UInt32 bytes
              = static_cast<UInt32>(sizeof(FileSystem::DirectoryEntry));

            if (bytes <= FileSystem::messageDataBytes) {
              for (UInt32 i = 0; i < bytes; ++i) {
                response.data[i] = reinterpret_cast<UInt8*>(&entry)[i];
              }

              response.dataLength = bytes;
              response.status = 0;
            }
          } else {
            FileSystem::DirectoryEntry emptyEntry {};
            UInt32 bytes
              = static_cast<UInt32>(sizeof(FileSystem::DirectoryEntry));

            if (bytes <= FileSystem::messageDataBytes) {
              for (UInt32 i = 0; i < bytes; ++i) {
                response.data[i] = reinterpret_cast<UInt8*>(&emptyEntry)[i];
              }

              response.dataLength = bytes;
              response.status = 0;
            }
          }
        }
      } else if (
        request.op == static_cast<UInt32>(FileSystem::Operation::Read)
      ) {
        HandleState* state = GetHandleState(request.arg0);

        if (!_volume || !state || !state->inUse || state->isDirectory) {
          response.status = 0;
        } else {
          UInt32 maxBytes = request.arg1;

          if (maxBytes > FileSystem::messageDataBytes) {
            maxBytes = FileSystem::messageDataBytes;
          }

          if (maxBytes == 0 || state->fileOffset >= state->fileSize) {
            response.status = 0;
          } else {
            UInt32 bytesRead = 0;

            if (
              _volume->ReadFile(
                state->startCluster,
                state->fileOffset,
                response.data,
                maxBytes,
                bytesRead,
                state->fileSize
              )
            ) {
              state->fileOffset += bytesRead;
              response.dataLength = bytesRead;
              response.status = bytesRead;
            } else {
              response.status = 0;
            }
          }
        }
      } else if (
        request.op == static_cast<UInt32>(FileSystem::Operation::Write)
      ) {
        HandleState* state = GetHandleState(request.arg0);

        if (!_volume || !state || !state->inUse || state->isDirectory) {
          response.status = 0;
        } else {
          UInt32 dataBytes = request.dataLength;

          if (dataBytes > FileSystem::messageDataBytes) {
            dataBytes = FileSystem::messageDataBytes;
          }

          if (dataBytes == 0) {
            response.status = 0;
          } else {
            UInt32 bytesWritten = 0;
            UInt32 newSize = state->fileSize;
            UInt32 startCluster = state->startCluster;

            if (
              _volume->WriteFileData(
                startCluster,
                state->fileOffset,
                request.data,
                dataBytes,
                bytesWritten,
                state->fileSize,
                newSize
              )
            ) {
              state->startCluster = startCluster;
              state->fileSize = newSize;
              state->fileOffset += bytesWritten;
              response.status = bytesWritten;

              if (state->entryLBA != 0) {
                _volume->UpdateEntry(
                  state->entryLBA,
                  state->entryOffset,
                  static_cast<UInt16>(startCluster),
                  newSize
                );
              }
            } else {
              response.status = 0;
            }
          }
        }
      } else if (
        request.op == static_cast<UInt32>(FileSystem::Operation::Stat)
      ) {
        HandleState* state = GetHandleState(request.arg0);

        if (!_volume || !state || !state->inUse) {
          response.status = 1;
        } else {
          FileSystem::FileInfo info {};

          info.sizeBytes = state->fileSize;
          info.attributes = state->attributes;

          UInt32 bytes
            = static_cast<UInt32>(sizeof(FileSystem::FileInfo));

          if (bytes <= FileSystem::messageDataBytes) {
            for (UInt32 i = 0; i < bytes; ++i) {
              response.data[i] = reinterpret_cast<UInt8*>(&info)[i];
            }

            response.dataLength = bytes;
            response.status = 0;
          }
        }
      } else if (
        request.op == static_cast<UInt32>(
          FileSystem::Operation::CreateDirectory
        )
      ) {
        CString path = reinterpret_cast<CString>(request.data);

        if (_volume && request.arg0 == _volume->GetHandle()) {
          UInt32 parentCluster = 0;
          bool parentIsRoot = true;
          char name[FileSystem::maxDirectoryLength] = {};

          if (
            ResolveParent(
              path,
              parentCluster,
              parentIsRoot,
              name,
              sizeof(name)
            )
          ) {
            if (_volume->CreateDirectory(parentCluster, parentIsRoot, name)) {
              response.status = 0;
            }
          }
        }
      } else if (
        request.op == static_cast<UInt32>(FileSystem::Operation::CreateFile)
      ) {
        CString path = reinterpret_cast<CString>(request.data);

        if (_volume && request.arg0 == _volume->GetHandle()) {
          UInt32 parentCluster = 0;
          bool parentIsRoot = true;
          char name[FileSystem::maxDirectoryLength] = {};

          if (
            ResolveParent(
              path,
              parentCluster,
              parentIsRoot,
              name,
              sizeof(name)
            )
          ) {
            if (_volume->CreateFile(parentCluster, parentIsRoot, name)) {
              response.status = 0;
            }
          }
        }
      } else if (
        request.op == static_cast<UInt32>(FileSystem::Operation::Remove)
      ) {
        CString path = reinterpret_cast<CString>(request.data);

        if (_volume && request.arg0 == _volume->GetHandle()) {
          UInt32 parentCluster = 0;
          bool parentIsRoot = true;
          char name[FileSystem::maxDirectoryLength] = {};

          if (
            ResolveParent(
              path,
              parentCluster,
              parentIsRoot,
              name,
              sizeof(name)
            )
          ) {
            if (_volume->RemoveEntry(parentCluster, parentIsRoot, name)) {
              response.status = 0;
            }
          }
        }
      } else if (
        request.op == static_cast<UInt32>(FileSystem::Operation::Rename)
      ) {
        CString fromPath = reinterpret_cast<CString>(request.data);

        if (_volume && request.arg0 == _volume->GetHandle()) {
          UInt32 offset = 0;

          while (offset < request.dataLength && request.data[offset] != 0) {
            ++offset;
          }

          if (offset + 1 < request.dataLength) {
            CString toPath
              = reinterpret_cast<CString>(request.data + offset + 1);
            UInt32 fromCluster = 0;
            UInt32 toCluster = 0;
            bool fromIsRoot = true;
            bool toIsRoot = true;
            char fromName[FileSystem::maxDirectoryLength] = {};
            char toName[FileSystem::maxDirectoryLength] = {};

            if (
              ResolveParent(
                fromPath,
                fromCluster,
                fromIsRoot,
                fromName,
                sizeof(fromName)
              ) &&
              ResolveParent(
                toPath,
                toCluster,
                toIsRoot,
                toName,
                sizeof(toName)
              )
            ) {
              if (fromCluster == toCluster && fromIsRoot == toIsRoot) {
                if (
                  _volume->RenameEntry(
                    fromCluster,
                    fromIsRoot,
                    fromName,
                    toName
                  )
                ) {
                  response.status = 0;
                }
              }
            }
          }
        }
      } else if (
        request.op == static_cast<UInt32>(FileSystem::Operation::Seek)
      ) {
        HandleState* state = GetHandleState(request.arg0);

        if (!_volume || !state || !state->inUse || state->isDirectory) {
          response.status = 0;
        } else {
          UInt32 origin = request.arg2;
          UInt32 base = 0;
          bool validOrigin = true;

          if (origin == 0) {
            base = 0;
          } else if (origin == 1) {
            base = state->fileOffset;
          } else if (origin == 2) {
            base = state->fileSize;
          } else {
            response.status = 0;
            validOrigin = false;
          }

          if (validOrigin) {
            UInt32 newOffset = base + request.arg1;

            // clamp to file size
            if (newOffset > state->fileSize) {
              newOffset = state->fileSize;
            }

            state->fileOffset = newOffset;
            response.status = newOffset;
          }
        }
      }

      IPC::Message reply {};

      reply.length = FileSystem::messageHeaderBytes + response.dataLength;

      if (reply.length > IPC::maxPayloadBytes) {
        continue;
      }

      for (UInt32 i = 0; i < reply.length; ++i) {
        reply.payload[i] = reinterpret_cast<UInt8*>(&response)[i];
      }

      IPC::Send(request.replyPortId, reply);
    }
  }
}
