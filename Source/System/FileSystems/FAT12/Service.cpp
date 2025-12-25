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

#define TEST // TODO #18

namespace Quantum::System::FileSystems::FAT12 {
  using Console = ABI::Console;
  using FileSystem = ABI::FileSystem;
  using IPC = ABI::IPC;
  using Task = ABI::Task;

  void Service::InitializeVolume() {
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
            const char* cursor = path;
            bool isRoot = true;
            bool ok = true;
            bool sawSegment = false;
            bool endsWithSeparator = false;

            if (path) {
              UInt32 len = 0;

              while (path[len] != '\0') {
                ++len;
              }

              if (len > 0) {
                char last = path[len - 1];

                endsWithSeparator = (last == '/' || last == '\\');
              }
            }

            while (*cursor == '/' || *cursor == '\\') {
              ++cursor;
            }

            while (*cursor != '\0') {
              char segment[FileSystem::maxDirectoryLength] = {};
              UInt32 length = 0;

              while (
                *cursor != '\0' &&
                *cursor != '/' &&
                *cursor != '\\' &&
                length + 1 < sizeof(segment)
              ) {
                segment[length++] = *cursor++;
              }

              segment[length] = '\0';
              sawSegment = true;

              bool hadSeparator = false;

              while (*cursor == '/' || *cursor == '\\') {
                ++cursor;
                hadSeparator = true;
              }

              if (segment[0] == '\0') {
                continue;
              }

              UInt32 nextCluster = 0;
              UInt8 attributes = 0;
              UInt32 sizeBytes = 0;
              bool lastSegment = (*cursor == '\0');
              bool requireDirectory = lastSegment && hadSeparator;

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

              if (endsWithSeparator && !isDirectory) {
                ok = false;
              } else if (isDirectory) {
                FileSystem::Handle handle = AllocateHandle(
                  true,
                  false,
                  lastCluster,
                  0
                );

                response.status = handle;
              } else if (lastCluster != 0 || lastSize == 0) {
                FileSystem::Handle handle = AllocateHandle(
                  false,
                  false,
                  lastCluster,
                  lastSize
                );

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
