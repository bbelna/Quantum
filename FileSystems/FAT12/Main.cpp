/**
 * @file FileSystems/FAT12/Main.cpp
 * @brief Entry point for the FAT12 file system backend process.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Core/CString.hpp>
#include <Quantum/Kernel.hpp>
#include <Quantum/Servers/Storage/ABI.hpp>
#include <Quantum/Threading/Thread.hpp>

#include "FAT12Server.hpp"

using namespace Quantum::FileSystems::FAT12;
using namespace Quantum::Kernel;

namespace StorABI = ::Quantum::Servers::Storage::ABI;

/**
 * @brief Maximum number of FAT12-capable devices to mount.
 */
static constexpr UInt32 MaxDevices = 8;

/**
 * @brief Discovered device info for passing to server threads.
 */
struct FAT12DeviceInfo {
  UInt32 DeviceID;
  char Name[32];
};

/**
 * @brief Device info discovered during enumeration.
 */
static FAT12DeviceInfo DiscoveredDevices[MaxDevices];

/**
 * @brief Number of devices found.
 */
static UInt32 DiscoveredDeviceCount = 0;

/**
 * @brief Thread entry point for an additional FAT12 server instance.
 * @param arg Index into the `DiscoveredDevices` array.
 */
static void AdditionalServerThread(UInt32 arg) {
  auto* server = new FAT12Server();

  server->Start(
    DiscoveredDevices[arg].DeviceID,
    DiscoveredDevices[arg].Name
  );
}

/**
 * @brief FAT12 server process entry point.
 *
 * Enumerates all floppy storage devices and starts a FAT12 server for
 * each one. The first device runs on the main thread; additional
 * devices run on spawned threads.
 */
int Main() {
  // wait for the Storage server, then enumerate all FAT12-capable devices
  // (floppy disks and hard disks)
  StorABI::GetDevicesResult deviceResult = {};

  for (;;) {
    if (StorABI::GetDevices(&deviceResult)) {
      for (
        UInt32 i = 0;
        i < deviceResult.Count && i < StorABI::MaxDevices;
        i++
      ) {
        const auto& descriptor = deviceResult.Devices[i];

        if (
          DiscoveredDeviceCount < MaxDevices &&
          (
            descriptor.Type == StorABI::StorageDeviceType::Floppy ||
            descriptor.Type == StorABI::StorageDeviceType::HardDisk
          )
        ) {
          DiscoveredDevices[DiscoveredDeviceCount].DeviceID
            = descriptor.DeviceID;

          Quantum::Core::CString::Copy(
            descriptor.Name,
            DiscoveredDevices[DiscoveredDeviceCount].Name,
            sizeof(DiscoveredDevices[DiscoveredDeviceCount].Name)
          );

          DiscoveredDeviceCount++;
        }
      }

      if (DiscoveredDeviceCount > 0) break;
    }

    ::Quantum::Threading::Thread::Yield();
  }

  // spawn threads for additional devices (index 1, 2, ...)
  for (UInt32 i = 1; i < DiscoveredDeviceCount; ++i) {
    ::Quantum::Threading::Thread::Create(AdditionalServerThread, i);
  }

  // run the first device on the main thread (blocks forever)
  FAT12Server primaryServer;

  primaryServer.Start(
    DiscoveredDevices[0].DeviceID,
    DiscoveredDevices[0].Name
  );

  return -1;
}
