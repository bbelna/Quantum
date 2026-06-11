/**
 * @file Drivers/Storage/FDC/FDCServer.cpp
 * @brief Implements @ref @QDrvs::Storage::FDC::FDCServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "FDCServer.hpp"

namespace Quantum::Drivers::Storage::FDC {
  FDCServer::FDCServer(
    StartupClient& startupClient,
    FDCStorageController& controller,
    FDCDriver& driver
  ) : Server(static_cast<Kernel::IPC::IPCPortID>(-1)) {
    StorageABI::StorageDeviceDescriptor descriptors[2];
    UInt32 deviceCount = 0;
    UInt8 driveNumbers[2] = {};

    if (driver.Probe()) {
      for (UInt8 driveIndex = 0; driveIndex < 2; driveIndex++) {
        if (!driver.DrivePresent(driveIndex)) continue;

        StorageABI::StorageDeviceDescriptor& descriptor =
          descriptors[deviceCount];

        descriptor.DeviceID = 0; // assigned by Storage server
        descriptor.Type = StorageABI::StorageDeviceType::Floppy;
        descriptor.SectorCount = FDCDriver::TotalSectors;
        descriptor.SectorSize = FDCDriver::SectorSize;
        descriptor.ReadOnly = false;
        descriptor.Removable = true;
        descriptor.Name[0] = 'F';
        descriptor.Name[1] = 'D';
        descriptor.Name[2] = 'C';
        descriptor.Name[3] = static_cast<char>('0' + driveIndex);
        descriptor.Name[4] = '\0';

        const char* prefix = "Floppy Disk ";
        UInt32 position = 0;

        while (prefix[position]) {
          descriptor.DisplayName[position] = prefix[position];
          position++;
        }

        descriptor.DisplayName[position] = static_cast<char>('A' + driveIndex);
        descriptor.DisplayName[position + 1] = '\0';

        driveNumbers[deviceCount] = driveIndex;
        deviceCount++;
      }
    }

    controller.SetDeviceMapping(deviceCount, driveNumbers);

    RegisterController(DriverABI::Operation::Read, controller);
    RegisterController(DriverABI::Operation::Write, controller);
    RegisterController(DriverABI::Operation::Flush, controller);
    RegisterController(DriverABI::Operation::GetMediaStatus, controller);

    if (
      deviceCount > 0 &&
      _portHandle != static_cast<Kernel::IPC::IPCPortResourceID>(-1)
    ) {
      StorageABI::RegisterDriver(_portID, deviceCount, descriptors);
    }

    startupClient.Ready();
  }
}

/**
 * @brief Entry point for @ref @QDrvs::Storage::FDC.
 * @return Non-zero error code on failure. Does **not** return on success.
 * @note This is a device driver and should not exit under normal operation.
 *       Returning from this function indicates a failure.
 *
 * Initializes @ref @QDrvs::Storage::FDC::FDCServer and enters the
 * server loop (@ref @QDrvs::Storage::FDC::FDCServer::Run).
 */
int Main() {
  KernelClient kernel;

  kernel.RequestPermission(
    ProcessPermissions::PortIO | ProcessPermissions::Interrupts
  );

  StartupClient startupClient;
  ServerLog log(kernel);
  FDCDriver driver(kernel);
  FDCStorageController controller(kernel, log, driver);
  FDCServer server(startupClient, controller, driver);

  return server.Run();
}
