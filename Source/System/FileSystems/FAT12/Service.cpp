/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/FileSystems/FAT12/Service.cpp
 * FAT12 file system service.
 */

#include <ABI/Console.hpp>
#include <ABI/Devices/BlockDevice.hpp>
#include <ABI/FileSystem.hpp>
#include <ABI/IPC.hpp>
#include <ABI/Task.hpp>

#include "Service.hpp"

namespace Quantum::System::FileSystems::FAT12 {
  using Console = ABI::Console;
  using BlockDevice = ABI::Devices::BlockDevice;
  using FileSystem = ABI::FileSystem;
  using IPC = ABI::IPC;
  using Task = ABI::Task;

  static constexpr FileSystem::VolumeHandle _volumeHandle = 1;

  // TODO: refactor into something reusable within the Quantum library
  static bool MatchLabel(CString label, CString expected) {
    if (!label || !expected) {
      return false;
    }

    UInt32 i = 0;

    while (label[i] != '\0' && expected[i] != '\0') {
      // case-insensitive compare for single-letter volume labels
      char a = label[i];
      char b = expected[i];

      if (a >= 'a' && a <= 'z') {
        a = static_cast<char>(a - 'a' + 'A');
      }

      if (b >= 'a' && b <= 'z') {
        b = static_cast<char>(b - 'a' + 'A');
      }

      if (a != b) {
        return false;
      }

      ++i;
    }

    if (label[i] == ':' && expected[i] == '\0' && label[i + 1] == '\0') {
      return true;
    }

    return label[i] == '\0' && expected[i] == '\0';
  }

  static bool GetFloppyInfo(BlockDevice::Info& outInfo) {
    UInt32 count = BlockDevice::GetCount();

    for (UInt32 i = 1; i <= count; ++i) {
      BlockDevice::Info info{};

      // find the first floppy device to bind to this service
      if (BlockDevice::GetInfo(i, info) != 0) {
        continue;
      }

      if (info.type == BlockDevice::Type::Floppy) {
        outInfo = info;

        return true;
      }
    }

    return false;
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

      if (request.op == static_cast<UInt32>(ABI::SystemCall::FileSystem_ListVolumes)) {
        UInt32 maxEntries = request.arg1;
        UInt32 count = maxEntries > 0 ? 1 : 0;

        if (count > 0 && sizeof(FileSystem::VolumeEntry) <= FileSystem::messageDataBytes) {
          // return a single "A" volume for now
          FileSystem::VolumeEntry entry{};

          entry.label[0] = 'A';
          entry.label[1] = '\0';
          entry.fsType = static_cast<UInt32>(FileSystem::Type::FAT12);

          UInt32 bytes = static_cast<UInt32>(sizeof(FileSystem::VolumeEntry));

          for (UInt32 i = 0; i < bytes; ++i) {
            response.data[i] = reinterpret_cast<UInt8*>(&entry)[i];
          }

          response.dataLength = bytes;
          response.status = count;
        } else {
          response.status = 0;
        }
      } else if (request.op == static_cast<UInt32>(ABI::SystemCall::FileSystem_OpenVolume)) {
        CString label = reinterpret_cast<CString>(request.data);

        // accept "A" or "A:" and return the fixed handle
        if (MatchLabel(label, "A")) {
          response.status = _volumeHandle;
        } else {
          response.status = 0;
        }
      } else if (request.op == static_cast<UInt32>(ABI::SystemCall::FileSystem_GetVolumeInfo)) {
        if (request.arg0 == _volumeHandle) {
          BlockDevice::Info blockInfo{};
          FileSystem::VolumeInfo info{};

          // surface basic geometry from the floppy block device
          info.label[0] = 'A';
          info.label[1] = '\0';
          info.fsType = static_cast<UInt32>(FileSystem::Type::FAT12);

          if (GetFloppyInfo(blockInfo)) {
            info.sectorSize = blockInfo.sectorSize;
            info.sectorCount = blockInfo.sectorCount;
          } else {
            info.sectorSize = 0;
            info.sectorCount = 0;
          }

          info.freeSectors = 0;

          UInt32 bytes = static_cast<UInt32>(sizeof(FileSystem::VolumeInfo));
          if (bytes <= FileSystem::messageDataBytes) {
            for (UInt32 i = 0; i < bytes; ++i) {
              response.data[i] = reinterpret_cast<UInt8*>(&info)[i];
            }

            response.dataLength = bytes;
            response.status = 0;
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
