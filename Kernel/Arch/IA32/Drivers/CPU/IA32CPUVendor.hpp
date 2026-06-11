/**
 * @file Kernel/Arch/IA32/Drivers/CPU/IA32CPUVendor.hpp
 * @brief Declares @ref @QKrnlIA32::Drivers::CPU::IA32CPUVendor and associated
 *        helpers.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Arch::IA32::Drivers::CPU {
  /**
   * @brief Known IA-32 CPU vendors, identified by CPUID leaf 0 vendor
   *        string.
   */
  enum class IA32CPUVendor : UInt8 {
    /**
     * @brief Unrecognized vendor string.
     */
    Unknown = 0,

    /**
     * @brief Intel (`"GenuineIntel"`).
     */
    Intel,

    /**
     * @brief AMD (`"AuthenticAMD"`).
     */
    AMD,

    /**
     * @brief VIA / Centaur (`"CentaurHauls"`).
     */
    VIA,

    /**
     * @brief Cyrix (`"CyrixInstead"`).
     */
    Cyrix,

    /**
     * @brief Transmeta (`"GenuineTMx86"`).
     */
    Transmeta,

    /**
     * @brief Hygon (`"HygonGenuine"`).
     */
    Hygon
  };

  /**
   * @brief Identifies the CPU vendor from a 12-character CPUID vendor
   *        string.
   * @param vendorString Null-terminated vendor string from CPUID leaf 0.
   * @return The matching vendor, or @ref IA32CPUVendor::Unknown.
   */
  IA32CPUVendor IA32CPUVendorFromString(const char* vendorString);

  /**
   * @brief Returns the short human-readable name for a vendor.
   * @param vendor The vendor to convert.
   * @return A short name such as `"Intel"`, `"AMD"`, etc.
   */
  const char* ToShortName(IA32CPUVendor vendor);

  /**
   * @brief Builds a device name from a vendor and CPU number.
   * @param vendor The CPU vendor.
   * @param cpuNumber Zero-based CPU index.
   * @param buffer Destination buffer.
   * @param bufferSize Size of the destination buffer.
   */
  void ToDeviceName(
    IA32CPUVendor vendor,
    UInt32 cpuNumber,
    char* buffer,
    Size bufferSize
  );

  /**
   * @brief Builds a user-friendly display name for the CPU.
   * @param vendor The CPU vendor.
   * @param brandString Null-terminated brand string from CPUID, or
   *                    `nullptr` if not available.
   * @param buffer Destination buffer.
   * @param bufferSize Size of the destination buffer.
   */
  void ToDeviceDisplayName(
    IA32CPUVendor vendor,
    const char* brandString,
    char* buffer,
    Size bufferSize
  );

  /**
   * @brief Builds a display name from vendor, family, and model when no
   *        brand string is available.
   * @param vendor The CPU vendor.
   * @param family CPU family number.
   * @param model CPU model number.
   * @param buffer Destination buffer.
   * @param bufferSize Size of the destination buffer.
   */
  void ToDeviceDisplayName(
    IA32CPUVendor vendor,
    UInt32 family,
    UInt32 model,
    char* buffer,
    Size bufferSize
  );
}
