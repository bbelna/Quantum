/**
 * @file Include/Quantum/HAL/Device.hpp
 * @brief Declaration of QuantumOS device structures and related types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Kernel.hpp>

namespace Quantum::HAL {
  /**
   * @brief Type for device category identifiers.
   */
  using DeviceCategoryID = UInt32;

  /**
   * @brief Maximum length for device category names, including the null
   *        terminator.
   */
  inline constexpr Size DeviceCategoryNameMaxLength = 64;

  /**
   * @brief Maximum length for device category display names, including the null
   *        terminator.
   */
  inline constexpr Size DeviceCategoryDisplayNameMaxLength = 64;

  /**
   * @brief Standard device categories.
   */
  enum class DeviceCategoryType : DeviceCategoryID {
    /**
     * @brief No category / invalid.
     */
    None = 0,

    /**
     * @brief Graphics devices.
     */
    Graphics = 1,

    /**
     * @brief Input devices (keyboard, mouse, etc.).
     */
    Input = 2,

    /**
     * @brief Storage devices (disks, flash, etc.).
     */
    Storage = 3,

    /**
     * @brief Network interface devices.
     */
    Network = 4,

    /**
     * @brief Audio devices.
     */
    Audio = 5,

    /**
     * @brief Bus controllers (PCI, USB, etc.).
     */
    Bus = 6,

    /**
     * @brief System devices (timers, DMA, interrupt controllers, etc.).
     */
    System = 7,

    /**
     * @brief Processor devices.
     */
    CPU = 8,

    /**
     * @brief Real-time clock devices.
     */
    RTC = 9,

    /**
     * @brief Chipset devices (northbridge, southbridge).
     */
    Chipset = 10,

    /**
     * @brief DMA controllers.
     */
    DMA = 11,

    /**
     * @brief Serial ports (COM1, COM2, etc.).
     */
    SerialPort = 12,

    /**
     * @brief Generic hardware controllers.
     */
    Controller = 13,

    /**
     * @brief First ID available for dynamically registered categories.
     */
    FirstDynamic = 256
  };

  /**
   * @brief Converts a `DeviceCategoryType` to a human-readable string.
   * @param categoryType The category type to convert.
   * @return A string representation of the category type.
   */
  constexpr const char* ToString(DeviceCategoryType categoryType) {
    switch (categoryType) {
      case DeviceCategoryType::None: return "None";
      case DeviceCategoryType::Graphics: return "Graphics";
      case DeviceCategoryType::Input: return "Input";
      case DeviceCategoryType::Storage: return "Storage";
      case DeviceCategoryType::Network: return "Network";
      case DeviceCategoryType::Audio: return "Audio";
      case DeviceCategoryType::Bus: return "Bus";
      case DeviceCategoryType::System: return "System";
      case DeviceCategoryType::CPU: return "CPU";
      case DeviceCategoryType::RTC: return "RTC";
      case DeviceCategoryType::Chipset: return "Chipset";
      case DeviceCategoryType::DMA: return "DMA";
      case DeviceCategoryType::SerialPort: return "Serial Port";
      case DeviceCategoryType::Controller: return "Controller";
      default: return "Unknown";
    }
  }

  /**
   * @brief Converts a `DeviceCategoryType` to a `DeviceCategoryID`.
   */
  constexpr DeviceCategoryID ToDeviceCategoryID(
    DeviceCategoryType categoryType
  ) {
    return static_cast<DeviceCategoryID>(categoryType);
  }

  // forward declaration for DeviceCategory
  struct Device;

  /**
   * @brief Maximum number of devices a single category can hold.
   */
  inline constexpr Size DeviceCategoryMaxDevices = 64;

  /**
   * @brief Category representing a class of devices.
   */
  struct DeviceCategory {
    /**
     * @brief Unique identifier for the device category.
     */
    DeviceCategoryID ID;

    /**
     * @brief Identifier of the parent category, or `DeviceCategoryType::None`
     *        if this is a root-level category.
     */
    DeviceCategoryID ParentID;

    /**
     * @brief Name of the device category (null-terminated string).
     */
    char Name[DeviceCategoryNameMaxLength];

    /**
     * @brief User-friendly display name for the device category
     *        (null-terminated string).
     */
    char DisplayName[DeviceCategoryDisplayNameMaxLength];

    /**
     * @brief Array of pointers to devices in this category.
     */
    Device* Devices[DeviceCategoryMaxDevices];

    /**
     * @brief Number of devices currently in this category.
     */
    Size DeviceCount;
  };

  /**
   * @brief Type for device identifiers.
   */
  using DeviceID = UInt32;

  /**
   * @brief Maximum length for device names, including the null terminator.
   */
  inline constexpr Size DeviceNameMaxLength = 64;

  /**
   * @brief Maximum length for device display names, including the null
   *        terminator.
   */
  inline constexpr Size DeviceDisplayNameMaxLength = 64;

  /**
   * @brief Lifecycle state of a device.
   */
  enum class DeviceState : UInt8 {
    /**
     * @brief The device has been discovered during enumeration but no driver
     *        has claimed it yet.
     */
    Discovered,

    /**
     * @brief A driver process has claimed the device but has not yet finished
     *        initializing it.
     */
    Bound,

    /**
     * @brief The driver has successfully initialized the device and it is
     *        ready for use.
     */
    Active,

    /**
     * @brief The device has been explicitly disabled.
     */
    Disabled,

    /**
     * @brief The driver reported a failure during initialization or operation.
     */
    Error
  };

  /**
   * @brief Maximum size of the opaque bus-specific data buffer on a device.
   */
  inline constexpr Size DeviceBusDataMaxSize = 64;

  /**
   * @brief Bus type constants. Platform-specific bus types (ISA, PCI, etc.)
   *        are defined in their own headers under `Bus/`.
   */
  namespace DeviceBus {
    /**
     * @brief No bus information.
     */
    constexpr UInt8 None = 0;

    /**
     * @brief Virtual device not attached to a physical bus.
     */
    constexpr UInt8 Virtual = 1;
  }

  /**
   * @brief Represents a device connected to the computer.
   */
  struct Device {
    /**
     * @brief Unique identifier for the device, assigned by the device server.
     */
    DeviceID ID;

    /**
     * @brief Name of the device (null-terminated string).
     */
    char Name[DeviceNameMaxLength];

    /**
     * @brief User-friendly display name for the device (null-terminated
     *        string).
     */
    char DisplayName[DeviceDisplayNameMaxLength];

    /**
     * @brief Category that this device belongs to.
     */
    DeviceCategoryID CategoryID;

    /**
     * @brief Current lifecycle state of the device.
     */
    DeviceState State;

    /**
     * @brief PID of the driver process that has claimed this device, or 0
     *        if no driver is currently bound.
     */
    ProcessID DriverPID;

    /**
     * @brief Identifier of the parent device in the device tree, or 0 for
     *        root-level (bus-level) devices.
     */
    DeviceID ParentID;

    /**
     * @brief The type of bus this device is attached to. Values are defined in
     *        the `DeviceBus` namespace and platform-specific bus headers.
     */
    UInt8 Bus;

    /**
     * @brief Number of bytes used in the `BusData` buffer.
     */
    UInt8 BusDataSize;

    /**
     * @brief Opaque bus-specific data. The layout is determined by the `Bus`
     *        field and interpreted by the corresponding bus header (e.g.
     *        `Bus/ISA.hpp`, `Bus/PCI.hpp`).
     */
    UInt8 BusData[DeviceBusDataMaxSize];
  };
}
