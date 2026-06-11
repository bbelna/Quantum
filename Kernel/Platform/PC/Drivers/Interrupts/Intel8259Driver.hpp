/**
 * @file Kernel/Platform/PC/Drivers/Interrupts/Intel8259Driver.hpp
 * @brief Declares @ref @QKrnlPC::Drivers::Interrupts::Intel8259Driver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/HAL/Platform/PC/Bus/ISA.hpp>

#include <Drivers/Interrupts/IInterruptControllerDriver.hpp>
#include <Drivers/DriverTypes.hpp>
#include <KernelTypes.hpp>

/**
 * @brief End-of-interrupt command code for the PIC.
 */
#define PIC_EOI 0x20

/**
 * @brief I/O port addresses for the PIC 1 command port.
 */
#define PIC1_COMMAND_PORT 0x20

/**
 * @brief I/O port address for the PIC 1 data port.
 */
#define PIC1_DATA_PORT 0x21

/**
 * @brief I/O port address for the PIC 2 command port.
 */
#define PIC2_COMMAND_PORT 0xA0

/**
 * @brief I/O port address for the PIC 2 data port.
 */
#define PIC2_DATA_PORT 0xA1

/**
 * @brief Initialization control word 1 (ICW1) flags.
 */
#define PIC_ICW1_INIT 0x10

/**
 * @brief ICW1 flag to indicate that ICW4 will be present.
 */
#define PIC_ICW1_ICW4 0x01

/**
 * @brief Initialization control word 4 (ICW4) flags.
 */
#define PIC_ICW4_8086 0x01

namespace Quantum::Kernel::Platform::PC::Drivers::Interrupts {
  /**
   * @brief Intel 8259 Programmable Interrupt Controller (PIC) driver.
   *
   * Manages the dual-PIC (master/slave) cascade found on PC-compatible
   * systems. The constructor remaps both PICs to the specified vector
   * offsets and masks all IRQ lines. Individual lines are then unmasked
   * as drivers are initialized.
   */
  class Intel8259Driver : public IInterruptControllerDriver<UInt8> {
    public:
      /**
       * @brief Creates a new `Intel8259Driver`.
       * @param cpu The CPU driver used for port I/O.
       * @param pic1Offset The interrupt vector offset for PIC 1.
       * @param pic2Offset The interrupt vector offset for PIC 2.
       */
      explicit Intel8259Driver(
        ICPUDriver& cpu,
        UInt32 pic1Offset,
        UInt32 pic2Offset
      );

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
       * @brief Invokes a driver-specific operation.
       * @param operation The operation code.
       * @param payload Optional payload pointer.
       * @return A driver-specific result value.
       */
      UInt32 Invoke(UInt32 operation, void* payload) override { return 0; }

      /**
       * @brief Sends an end-of-interrupt (EOI) signal for the given vector.
       * @param vector The interrupt vector to acknowledge.
       *
       * Writes the PIC EOI command to the appropriate PIC controller(s).
       * If the vector belongs to PIC 2 (slave), both the slave and master
       * PICs receive an EOI.
       */
      void End(UInt8 vector) override;

      /**
       * @brief Masks (disables) the given interrupt vector.
       * @param vector The interrupt vector to mask.
       */
      void Mask(UInt8 vector) override;

      /**
       * @brief Unmasks (enables) all interrupt vectors.
       */
      void MaskAll() override;

      /**
       * @brief Unmasks (enables) the given interrupt vector.
       * @param vector The interrupt vector to unmask.
       */
      void Unmask(UInt8 vector) override;

      /**
       * @brief Unmasks (enables) all interrupt vectors.
       */
      void UnmaskAll() override;

      /**
       * @brief Returns the base interrupt vector for hardware IRQs.
       * @return The PIC 1 offset (typically 0x20).
       */
      UInt8 GetBaseVector() override {
        return static_cast<UInt8>(_pic1Offset);
      }

    private:
      /**
       * @brief The CPU driver used for port I/O.
       */
      ICPUDriver& _cpu;

      /**
       * @brief The device associated with this driver.
       */
      Device _device {
        3,
        "Intel8259",
        "Intel 8259 Programmable Interrupt Controller",
        ToDeviceCategoryID(DeviceCategoryType::Controller),
        DeviceState::Active,
        0,
        0,
        DeviceBus::ISA,
        0,
        {}
      };

      /**
       * @brief PIC 1 interrupt vector offset.
       */
      UInt32 _pic1Offset = 0;

      /**
       * @brief PIC 2 interrupt vector offset.
       */
      UInt32 _pic2Offset = 0;

  };
}
