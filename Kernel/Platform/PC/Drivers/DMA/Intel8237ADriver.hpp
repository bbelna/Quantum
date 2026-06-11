/**
 * @file Kernel/Platform/PC/Drivers/DMA/Intel8237ADriver.hpp
 * @brief Declares @ref @QKrnlPC::Drivers::DMA::Intel8237ADriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/HAL/Platform/PC/Bus/ISA.hpp>

#include <Drivers/CPU/ICPUDriver.hpp>
#include <Drivers/DriverTypes.hpp>
#include <KernelTypes.hpp>

namespace Quantum::Kernel::Platform::PC::Drivers::DMA {
  /**
   * @brief ISA DMA controller driver for the dual 8237A DMA controllers.
   *
   * Probes the two 8237A DMA controllers that are always present on
   * ISA-compatible machines. DMA1 handles 8-bit transfers on channels 0-3
   * and DMA2 handles 16-bit transfers on channels 4-7. The probe verifies
   * controller presence by writing and reading back address registers.
   */
  class Intel8237ADriver : public IDriver {
    public:
      /**
       * @brief DMA1 flip-flop reset port.
       *
       * Writing any value to this port resets the internal byte pointer
       * (flip-flop) of the DMA1 controller, so the next read or write to a
       * 16-bit register accesses the low byte first.
       */
      static constexpr UInt16 DMA1FlipFlop = 0x0C;

      /**
       * @brief DMA1 channel 1 base address register port.
       *
       * A 16-bit register accessed as two consecutive byte writes/reads
       * (low byte first after a flip-flop reset). Used during the probe to
       * verify that DMA1 is present.
       */
      static constexpr UInt16 DMA1Channel1Address = 0x02;

      /**
       * @brief DMA1 single-channel mask register port.
       *
       * Writing bits `[1:0]` selects the channel and bit 2 sets (1) or
       * clears (0) the mask for that channel. Used to mask channel 1 before
       * probing and unmask it afterwards.
       */
      static constexpr UInt16 DMA1Mask = 0x0A;

      /**
       * @brief DMA2 flip-flop reset port.
       *
       * Writing any value to this port resets the internal byte pointer
       * (flip-flop) of the DMA2 controller.
       */
      static constexpr UInt16 DMA2FlipFlop = 0xD8;

      /**
       * @brief Constructs the DMA driver and probes the 8237A controllers.
       */
      explicit Intel8237ADriver();

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
       * @brief Invokes a DMA driver operation.
       * @param operation The operation code (reserved for future use).
       * @param payload Pointer to an operation-specific payload.
       * @return Operation-specific result; currently always `0`.
       */
      UInt32 Invoke(UInt32 operation, void* payload) override;

      /**
       * @brief Returns whether the DMA controllers were detected.
       * @return `true` if both DMA1 and DMA2 are present.
       */
      bool IsPresent() const { return _present; }

    private:
      /**
       * @brief The device associated with this driver.
       */
      Device _device {
        10,
        "Intel8237A",
        "Intel 8237A DMA Controller",
        ToDeviceCategoryID(DeviceCategoryType::DMA),
        DeviceState::Discovered,
        0,
        0,
        DeviceBus::ISA,
        0,
        {}
      };

      /**
       * @brief Whether the DMA controllers were detected.
       */
      bool _present = false;

      /**
       * @brief Probes DMA1 and DMA2 by writing and reading back address
       *        registers. Called once from the constructor.
       */
      void _probe();
  };
}
