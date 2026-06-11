/**
 * @file Kernel/Platform/PC/Drivers/Bus/PCI.hpp
 * @brief Declares @ref @QKrnlPC::Drivers::Bus::PCI.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/HAL/PCI.hpp>

#include <Drivers/CPU/ICPUDriver.hpp>
#include <Drivers/DriverTypes.hpp>

namespace Quantum::Kernel::Platform::PC::Drivers::Bus {
  /**
   * @brief I/O port address for PCI configuration address register.
   */
  static constexpr UInt16 PCIConfigAddress = 0xCF8;

  /**
   * @brief I/O port address for PCI configuration data register (32-bit
   *        access).
   */
  static constexpr UInt16 PCIConfigData = 0xCFC;

  /**
   * @brief Information about a discovered PCI device (kernel-internal).
   */
  struct PCIDeviceInfo {
    /**
     * @brief PCI bus number (0-255).
     */
    UInt8 Bus;

    /**
     * @brief Device slot number (0-31).
     */
    UInt8 Slot;

    /**
     * @brief Function number (0-7).
     */
    UInt8 Function;

    /**
     * @brief PCI vendor ID.
     */
    UInt16 VendorID;

    /**
     * @brief PCI device ID.
     */
    UInt16 DeviceID;

    /**
     * @brief PCI BAR0-5 values (32-bit each).
     *
     * For memory BARs, the lower 4 bits are flags and the upper 28 bits are the
     * base address (must be masked with 0xFFFFFFF0). For I/O BARs, the lower 2
     * bits are flags and the upper 30 bits are the base address (must be masked
     * with 0xFFFFFFFC).
     */
    UInt32 BAR[6];
  };

  /**
   * @brief Kernel-level PC-platform PCI configuration space driver.
   *
   * Accesses PCI configuration registers via the legacy I/O port mechanism
   * (CONFIG_ADDRESS at 0xCF8, CONFIG_DATA at 0xCFC). Provides low-level
   * read/write helpers and higher-level convenience methods for probing
   * vendor/device IDs, BARs, and scanning for specific devices.
   *
   * Implements @ref IDriver so that userspace drivers can invoke PCI
   * operations through the platform driver dispatch path using
   * @ref HAL::PCI::PCIDriverOperation codes.
   */
  class PCI : public IDriver {
    public:
      /**
       * @brief Creates a new @ref PCI driver.
       * @param cpu Reference to the CPU driver used for I/O port access.
       */
      explicit PCI(ICPUDriver& cpu);

      // ----- IDriver interface -----

      /**
       * @brief Gets the device associated with this driver.
       * @return The device associated with this driver.
       */
      Device& GetDevice() override { return _device; }

      /**
       * @brief Gets the unique identifier of the device.
       * @return The device ID.
       */
      DeviceID GetDeviceID() override { return _device.ID; }

      /**
       * @brief Invokes a PCI driver operation.
       * @param operation The operation code (see @ref
       *        HAL::PCI::PCIDriverOperation).
       * @param payload Pointer to an operation-specific payload.
       * @return Operation-specific result.
       */
      UInt32 Invoke(UInt32 operation, void* payload) override;

      // ----- Direct config space access -----

      /**
       * @brief Reads a 32-bit value from PCI configuration space.
       * @param bus PCI bus number (0-255).
       * @param slot Device slot number (0-31).
       * @param func Function number (0-7).
       * @param offset Register offset (must be dword-aligned).
       * @return The 32-bit configuration register value.
       */
      UInt32 ReadConfig(UInt8 bus, UInt8 slot, UInt8 func, UInt8 offset);

      /**
       * @brief Writes a 32-bit value to PCI configuration space.
       * @param bus PCI bus number (0-255).
       * @param slot Device slot number (0-31).
       * @param func Function number (0-7).
       * @param offset Register offset (must be dword-aligned).
       * @param value The 32-bit value to write.
       */
      void WriteConfig(
        UInt8 bus,
        UInt8 slot,
        UInt8 func,
        UInt8 offset,
        UInt32 value
      );

      /**
       * @brief Reads the vendor ID from a PCI device.
       * @param bus PCI bus number (0-255).
       * @param slot Device slot number (0-31).
       * @param func Function number (0-7).
       * @return The 16-bit vendor ID, or 0xFFFF if no device present.
       */
      UInt16 ReadVendorID(UInt8 bus, UInt8 slot, UInt8 func);

      /**
       * @brief Reads the device ID from a PCI device.
       * @param bus PCI bus number (0-255).
       * @param slot Device slot number (0-31).
       * @param func Function number (0-7).
       * @return The 16-bit device ID.
       */
      UInt16 ReadDeviceID(UInt8 bus, UInt8 slot, UInt8 func);

      /**
       * @brief Reads a Base Address Register (BAR) from a PCI device.
       * @param bus PCI bus number (0-255).
       * @param slot Device slot number (0-31).
       * @param func Function number (0-7).
       * @param barIndex BAR index (0-5).
       * @return The 32-bit BAR value.
       */
      UInt32 ReadBAR(UInt8 bus, UInt8 slot, UInt8 func, UInt8 barIndex);

      /**
       * @brief Reads the header type from a PCI device.
       * @param bus PCI bus number (0-255).
       * @param slot Device slot number (0-31).
       * @param func Function number (0-7).
       * @return The 8-bit header type.
       */
      UInt8 ReadHeaderType(UInt8 bus, UInt8 slot, UInt8 func);

      /**
       * @brief Scans PCI bus 0 for a device matching the given vendor and
       *        device ID.
       * @param vendorID The PCI vendor ID to search for.
       * @param deviceID The PCI device ID to search for.
       * @param out Pointer to a `PCIDeviceInfo` struct to fill on success.
       * @return `true` if the device was found, false otherwise.
       */
      bool FindDevice(UInt16 vendorID, UInt16 deviceID, PCIDeviceInfo* out);

      /**
       * @brief Scans PCI bus 0 for a device matching the given class and
       *        subclass code.
       * @param classCode The PCI class code (e.g. `0x01` for mass storage).
       * @param subclassCode The PCI subclass code (e.g. `0x01` for IDE).
       * @param out Pointer to an @ref HAL::PCI::PCIDeviceInfo struct to fill
       *        on success.
       * @return `true` if a matching device was found.
       */
      bool FindDeviceByClass(
        UInt8 classCode,
        UInt8 subclassCode,
        HAL::PCI::PCIDeviceInfo* out
      );

    private:
      /**
       * @brief Reference to the CPU driver used for I/O port access.
       */
      ICPUDriver& _cpu;

      /**
       * @brief The device associated with this driver.
       */
      Device _device {
        11,
        "PCI",
        "PCI Configuration Space",
        ToDeviceCategoryID(DeviceCategoryType::Bus),
        DeviceState::Active,
        0,
        0,
        DeviceBus::None,
        0,
        {}
      };

      /**
       * @brief Populates an @ref HAL::PCI::PCIDeviceInfo struct from a
       *        PCI bus/slot/function address.
       * @param bus PCI bus number.
       * @param slot Device slot number.
       * @param func Function number.
       * @param out Pointer to the struct to fill.
       */
      void _fillDeviceInfo(
        UInt8 bus,
        UInt8 slot,
        UInt8 func,
        HAL::PCI::PCIDeviceInfo* out
      );
  };
}
