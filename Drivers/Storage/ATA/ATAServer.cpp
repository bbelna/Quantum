/**
 * @file Drivers/Storage/ATA/ATAServer.cpp
 * @brief Implements @ref @QDrvs::Storage::ATA::ATAServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "ATAServer.hpp"

namespace Quantum::Drivers::Storage::ATA {
  ATAServer::ATAServer(
    StartupClient& startupClient,
    ATAStorageController& controller,
    ATADriver& driver
  ) : Server(static_cast<Kernel::IPC::IPCPortID>(-1)) {
    // build device descriptors for registration
    StorageABI::StorageDeviceDescriptor devices[StorageABI::MaxDevicesPerDriver];
    UInt32 deviceCount = 0;

    static const char* const channelLabel[4] = {
      "Primary ",
      "Primary ",
      "Secondary ",
      "Secondary "
    };
    static const char* const driveLabel[4] = {
      "Master",
      "Slave",
      "Master",
      "Slave"
    };

    for (
      UInt8 index = 0;
      index < 4 && deviceCount < StorageABI::MaxDevicesPerDriver;
      index++
    ) {
      const ATADriver::DriveInfo& info = driver.GetDriveInfo(index);

      if (!info.Present) continue;

      StorageABI::StorageDeviceDescriptor& descriptor = devices[deviceCount];

      descriptor.DeviceID = 0;
      descriptor.Type = info.Type;
      descriptor.SectorCount = info.SectorCount;
      descriptor.SectorSize = 512;
      descriptor.ReadOnly = (
        info.Type == StorageABI::StorageDeviceType::OpticalDisk
      );
      descriptor.Removable = (
        info.Type == StorageABI::StorageDeviceType::OpticalDisk
      );

      // short name: "ATA0P0", "ATA0P1", "ATA1P0", "ATA1P1"
      descriptor.Name[0] = 'A';
      descriptor.Name[1] = 'T';
      descriptor.Name[2] = 'A';
      descriptor.Name[3] = static_cast<char>('0' + (index >> 1));
      descriptor.Name[4] = 'P';
      descriptor.Name[5] = static_cast<char>('0' + (index & 1));
      descriptor.Name[6] = '\0';

      // display name: "Primary Master", "Primary Slave", etc.
      UInt32 position = 0;

      for (
        const char* source = channelLabel[index];
        *source && position < 47;
        source++
      ) {
        descriptor.DisplayName[position++] = *source;
      }

      for (
        const char* source = driveLabel[index];
        *source && position < 47;
        source++
      ) {
        descriptor.DisplayName[position++] = *source;
      }

      descriptor.DisplayName[position] = '\0';

      deviceCount++;
    }

    RegisterController(DriverABI::Operation::Read, controller);
    RegisterController(DriverABI::Operation::Write, controller);
    RegisterController(DriverABI::Operation::Flush, controller);
    RegisterController(DriverABI::Operation::GetMediaStatus, controller);

    // send registration to Storage server (spin until port 8 opens)
    if (
      deviceCount > 0 &&
      _portHandle != static_cast<Kernel::IPC::IPCPortResourceID>(-1)
    ) {
      StorageABI::RegisterDriver(_portID, deviceCount, devices);
    }

    startupClient.Ready();
  }
}

/**
 * @brief Entry point for @ref @QDrvs::Storage::ATA.
 * @return Non-zero error code on failure. Does **not** return on success.
 * @note This is a device driver and should not exit under normal operation.
 *       Returning from this function indicates a failure.
 *
 * Initializes @ref @QDrvs::Storage::ATA::ATAServer and enters the
 * server loop (@ref @QDrvs::Storage::ATA::ATAServer::Run).
 */
int Main() {
  KernelClient kernel;

  kernel.RequestPermission(
    ProcessPermissions::PortIO | ProcessPermissions::Interrupts
  );

  StartupClient startupClient;
  ServerLog log(kernel);
  ATADriver driver(kernel);

  driver.Probe();

  ATAStorageController controller(kernel, log, driver);
  ATAServer server(startupClient, controller, driver);

  return server.Run();
}
