/**
 * @file Kernel/Platform/PC/Drivers/RTC/MC146818Driver.hpp
 * @brief Declares @ref @QKrnlPC::Drivers::RTC::MC146818Driver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/HAL/Platform/PC/Bus/ISA.hpp>

#include <Drivers/DriverTypes.hpp>
#include <KernelTypes.hpp>

namespace Quantum::Kernel::Platform::PC::Drivers::RTC {
  /**
   * @brief Date and time snapshot read from the RTC.
   */
  struct RTCDateTime {
    /**
     * @brief Seconds (`0`-`59`).
     */
    UInt8 Second;

    /**
     * @brief Minutes (`0`-`59`).
     */
    UInt8 Minute;

    /**
     * @brief Hours (`0`-`23`).
     */
    UInt8 Hour;

    /**
     * @brief Day of the month (`1`-`31`).
     */
    UInt8 Day;

    /**
     * @brief Month (`1`-`12`).
     */
    UInt8 Month;

    /**
     * @brief Full four-digit year (e.g. `2026`).
     */
    UInt16 Year;
  };

  /**
   * @brief MC146818-compatible Real-Time Clock (RTC) driver.
   *
   * Reads the current date and time from the CMOS RTC at I/O ports
   * `0x70`/`0x71`. The RTC is probed once at construction time; the
   * result is cached in an @ref RTCDateTime and served via
   * @ref GetDateTime.
   */
  class MC146818Driver : public IDriver {
    public:
      /**
       * @brief CMOS index/address port.
       *
       * Bit 7 controls NMI masking; all accesses OR the register index
       * with `0x80` to keep NMI disabled during reads.
       */
      static constexpr UInt16 IndexPort = 0x70;

      /**
       * @brief CMOS data port.
       */
      static constexpr UInt16 DataPort = 0x71;

      /**
       * @brief Constructs the RTC driver and probes the current time.
       */
      explicit MC146818Driver();

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
       * @brief Invokes an RTC driver operation.
       * @param operation The operation code.
       * @param payload Pointer to an operation-specific payload.
       * @return Operation-specific result.
       */
      UInt32 Invoke(UInt32 operation, void* payload) override;

      /**
       * @brief Returns the cached date/time snapshot.
       * @return A reference to the cached @ref RTCDateTime.
       */
      const RTCDateTime& GetDateTime() const { return _dateTime; }

    private:
      /**
       * @brief The device associated with this driver.
       */
      Device _device {
        9,
        "MC146818",
        "MC146818 Real-Time Clock",
        ToDeviceCategoryID(DeviceCategoryType::RTC),
        DeviceState::Active,
        0,
        0,
        DeviceBus::ISA,
        0,
        {}
      };

      /**
       * @brief Cached date/time read at probe time.
       */
      RTCDateTime _dateTime = {};

      /**
       * @brief Probes the RTC registers and populates @ref _dateTime.
       *
       * Waits for the Update In Progress bit to clear, reads all time
       * registers, converts from BCD if necessary, and reconstructs the
       * full four-digit year.
       */
      void _probe();

      /**
       * @brief Reads a single CMOS register.
       * @param reg The register index (`0x00`-`0x3F`).
       * @return The byte value of the register.
       *
       * NMI is kept disabled by OR-ing the index with `0x80`.
       */
      UInt8 _readRegister(UInt8 reg);

      /**
       * @brief Converts a BCD-encoded byte to binary.
       * @param bcd The BCD-encoded value.
       * @return The equivalent binary value.
       */
      static UInt8 _bcdToBinary(UInt8 bcd);
  };
}
