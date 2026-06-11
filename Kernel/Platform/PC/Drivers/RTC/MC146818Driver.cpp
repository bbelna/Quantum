/**
 * @file Kernel/Platform/PC/Drivers/RTC/MC146818Driver.cpp
 * @brief Implements @ref @QKrnlPC::Drivers::RTC::MC146818Driver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <KernelLog.hpp>

#include "MC146818Driver.hpp"

namespace Quantum::Kernel::Platform::PC::Drivers::RTC {
  MC146818Driver::MC146818Driver() {
    SetISAAddress(_device, { IndexPort, 0 });
    _probe();
  }

  UInt8 MC146818Driver::_readRegister(UInt8 reg) {
    UInt8 outVal = static_cast<UInt8>(0x80 | reg);
    asm volatile("outb %0, %1" :: "a"(outVal), "Nd"(IndexPort));

    UInt8 val;
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(DataPort));
    return val;
  }

  UInt8 MC146818Driver::_bcdToBinary(UInt8 bcd) {
    return static_cast<UInt8>(((bcd >> 4) * 10) + (bcd & 0x0F));
  }

  void MC146818Driver::_probe() {
    // wait for any update-in-progress to finish (Register A, bit 7)
    while (_readRegister(0x0A) & 0x80) {}

    UInt8 second = _readRegister(0x00);
    UInt8 minute = _readRegister(0x02);
    UInt8 hour = _readRegister(0x04);
    UInt8 day = _readRegister(0x07);
    UInt8 month = _readRegister(0x08);
    UInt8 year = _readRegister(0x09);

    // Register B: bit 2 = binary mode (1) or BCD mode (0)
    UInt8 registerB = _readRegister(0x0B);
    bool isBinaryMode = (registerB & 0x04) != 0;

    if (!isBinaryMode) {
      second = _bcdToBinary(second);
      minute = _bcdToBinary(minute);
      hour = _bcdToBinary(hour);
      day = _bcdToBinary(day);
      month = _bcdToBinary(month);
      year = _bcdToBinary(year);
    }

    // attempt to read century from register 0x32 (common ACPI convention)
    UInt8 centuryRaw = _readRegister(0x32);
    UInt16 fullYear;

    if (centuryRaw == 0x19 || centuryRaw == 0x20) {
      // valid BCD century
      UInt8 century = _bcdToBinary(centuryRaw);

      fullYear = static_cast<UInt16>(century * 100 + year);
    } else {
      // century register not usable; infer from two-digit year
      if (year >= 80) {
        fullYear = static_cast<UInt16>(1900 + year);
      } else {
        fullYear = static_cast<UInt16>(2000 + year);
      }
    }

    _dateTime.Second = second;
    _dateTime.Minute = minute;
    _dateTime.Hour = hour;
    _dateTime.Day = day;
    _dateTime.Month = month;
    _dateTime.Year = fullYear;

    KLOG_TRACE(
      "RTC reports %u-%u-%u %u:%u:%u",
      fullYear,
      month,
      day,
      hour,
      minute,
      second
    );
  }

  UInt32 MC146818Driver::Invoke(UInt32 operation, void* payload) {
    (void)operation;
    (void)payload;

    return 0;
  }
}
