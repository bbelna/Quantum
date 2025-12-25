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

#define TEST

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

  FileSystem::Handle Service::AllocateHandle() {
    for (UInt32 i = 0; i < _maxHandles; ++i) {
      if (!_handles[i].inUse) {
        _handles[i].inUse = true;
        _handles[i].nextIndex = 0;

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
    state->nextIndex = 0;
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
            FileSystem::Handle handle = AllocateHandle();

            response.status = handle;
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

        if (!_volume || !state || !state->inUse) {
          response.status = 1;
        } else {
          FileSystem::DirectoryEntry entry {};

          bool found = false;

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
