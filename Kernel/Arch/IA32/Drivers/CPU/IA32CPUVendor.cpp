/**
 * @file Kernel/Arch/IA32/Drivers/CPU/IA32CPUVendor.cpp
 * @brief Implements @ref @QKrnlIA32::Drivers::CPU::IA32CPUVendor
 *        helpers.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Core/CString.hpp>

#include <KernelTypes.hpp>

#include "IA32CPUVendor.hpp"

namespace Quantum::Kernel::Arch::IA32::Drivers::CPU {
  /**
   * @brief Maps a 12-character CPUID vendor string to an
   *        @ref IA32CPUVendor enumerator.
   */
  struct VendorEntry {
    /**
     * @brief The 12-character CPUID leaf 0 vendor string.
     */
    const char* String;

    /**
     * @brief The corresponding vendor enumerator.
     */
    IA32CPUVendor Vendor;
  };

  /**
   * @brief Lookup table of known CPUID vendor strings.
   */
  static constexpr VendorEntry VendorTable[] = {
    {"GenuineIntel", IA32CPUVendor::Intel},
    {"AuthenticAMD", IA32CPUVendor::AMD},
    {"CentaurHauls", IA32CPUVendor::VIA},
    {"CyrixInstead", IA32CPUVendor::Cyrix},
    {"GenuineTMx86", IA32CPUVendor::Transmeta},
    {"TransmetaCPU", IA32CPUVendor::Transmeta},
    {"HygonGenuine", IA32CPUVendor::Hygon}
  };

  IA32CPUVendor IA32CPUVendorFromString(const char* vendorString) {
    for (const VendorEntry& entry : VendorTable) {
      if (CString::Equals(vendorString, entry.String, 12)) {
        return entry.Vendor;
      }
    }

    return IA32CPUVendor::Unknown;
  }

  const char* ToShortName(IA32CPUVendor vendor) {
    switch (vendor) {
      case IA32CPUVendor::Intel: return "Intel";
      case IA32CPUVendor::AMD: return "AMD";
      case IA32CPUVendor::VIA: return "VIA";
      case IA32CPUVendor::Cyrix: return "Cyrix";
      case IA32CPUVendor::Transmeta: return "Transmeta";
      case IA32CPUVendor::Hygon: return "Hygon";
      default: return "IA32";
    }
  }

  void ToDeviceName(
    IA32CPUVendor vendor,
    UInt32 cpuNumber,
    char* buffer,
    Size bufferSize
  ) {
    CString::Format(
      buffer,
      bufferSize,
      "%sCPU%u",
      ToShortName(vendor),
      cpuNumber
    );
  }

  void ToDeviceDisplayName(
    IA32CPUVendor vendor,
    const char* brandString,
    char* buffer,
    Size bufferSize
  ) {
    if (brandString && brandString[0] != '\0') {
      CString::Copy(brandString, buffer, bufferSize);
    } else {
      CString::Copy(ToShortName(vendor), buffer, bufferSize);
    }
  }

  /**
   * @brief Maps a vendor, CPUID family, and model number to a
   *        human-readable CPU product name.
   */
  struct FamilyModelName {
    /**
     * @brief CPU vendor.
     */
    IA32CPUVendor Vendor;

    /**
     * @brief CPUID family (display family after extended family
     *        adjustment).
     */
    UInt32 Family;

    /**
     * @brief CPUID model (display model after extended model
     *        adjustment).
     */
    UInt32 Model;

    /**
     * @brief Human-readable product name.
     */
    const char* Name;
  };

  /**
   * @brief Lookup table of well-known CPU names for models that predate
   *        the brand string CPUID leaves (0x80000002-0x80000004).
   */
  static constexpr FamilyModelName KnownCPUs[] = {
    // Intel 486
    {IA32CPUVendor::Intel, 4, 0, "Intel 486DX-25/33"},
    {IA32CPUVendor::Intel, 4, 1, "Intel 486DX-50"},
    {IA32CPUVendor::Intel, 4, 2, "Intel 486SX"},
    {IA32CPUVendor::Intel, 4, 3, "Intel 486DX2"},
    {IA32CPUVendor::Intel, 4, 4, "Intel 486SL"},
    {IA32CPUVendor::Intel, 4, 5, "Intel 486SX2"},
    {IA32CPUVendor::Intel, 4, 7, "Intel 486DX2-WB"},
    {IA32CPUVendor::Intel, 4, 8, "Intel 486DX4"},
    {IA32CPUVendor::Intel, 4, 9, "Intel 486DX4-WB"},

    // Intel Pentium (P5)
    {IA32CPUVendor::Intel, 5, 1, "Intel Pentium (60/66)"},
    {IA32CPUVendor::Intel, 5, 2, "Intel Pentium (75-200)"},
    {IA32CPUVendor::Intel, 5, 3, "Intel Pentium OverDrive"},
    {IA32CPUVendor::Intel, 5, 4, "Intel Pentium MMX"},
    {IA32CPUVendor::Intel, 5, 7, "Intel Pentium (P54C)"},
    {IA32CPUVendor::Intel, 5, 8, "Intel Pentium MMX (Tillamook)"},

    // Intel Pentium Pro / II / III (P6)
    {IA32CPUVendor::Intel, 6, 1, "Intel Pentium Pro"},
    {IA32CPUVendor::Intel, 6, 3, "Intel Pentium II (Klamath)"},
    {IA32CPUVendor::Intel, 6, 5, "Intel Pentium II (Deschutes)"},
    {IA32CPUVendor::Intel, 6, 6, "Intel Celeron (Mendocino)"},
    {IA32CPUVendor::Intel, 6, 7, "Intel Pentium III (Katmai)"},
    {IA32CPUVendor::Intel, 6, 8, "Intel Pentium III (Coppermine)"},
    {IA32CPUVendor::Intel, 6, 9, "Intel Pentium M (Banias)"},
    {IA32CPUVendor::Intel, 6, 10, "Intel Pentium III Xeon"},
    {IA32CPUVendor::Intel, 6, 11, "Intel Pentium III (Tualatin)"},
    {IA32CPUVendor::Intel, 6, 13, "Intel Pentium M (Dothan)"},

    // AMD 486 / K5 / K6
    {IA32CPUVendor::AMD, 4, 3, "AMD Am486DX2"},
    {IA32CPUVendor::AMD, 4, 7, "AMD Am486DX2-WB"},
    {IA32CPUVendor::AMD, 4, 8, "AMD Am486DX4"},
    {IA32CPUVendor::AMD, 4, 9, "AMD Am486DX4-WB"},
    {IA32CPUVendor::AMD, 4, 14, "AMD Am5x86"},
    {IA32CPUVendor::AMD, 4, 15, "AMD Am5x86-WB"},
    {IA32CPUVendor::AMD, 5, 0, "AMD K5 (SSA/5)"},
    {IA32CPUVendor::AMD, 5, 1, "AMD K5"},
    {IA32CPUVendor::AMD, 5, 2, "AMD K5"},
    {IA32CPUVendor::AMD, 5, 3, "AMD K5"},
    {IA32CPUVendor::AMD, 5, 6, "AMD K6"},
    {IA32CPUVendor::AMD, 5, 7, "AMD K6 (Little Foot)"},
    {IA32CPUVendor::AMD, 5, 8, "AMD K6-2"},
    {IA32CPUVendor::AMD, 5, 9, "AMD K6-III"},
    {IA32CPUVendor::AMD, 5, 13, "AMD K6-2+/K6-III+"},

    // AMD Athlon (K7)
    {IA32CPUVendor::AMD, 6, 1, "AMD Athlon (K7)"},
    {IA32CPUVendor::AMD, 6, 2, "AMD Athlon (K75)"},
    {IA32CPUVendor::AMD, 6, 3, "AMD Duron (Spitfire)"},
    {IA32CPUVendor::AMD, 6, 4, "AMD Athlon (Thunderbird)"},
    {IA32CPUVendor::AMD, 6, 6, "AMD Athlon MP/XP (Palomino)"},
    {IA32CPUVendor::AMD, 6, 7, "AMD Duron (Morgan)"},
    {IA32CPUVendor::AMD, 6, 8, "AMD Athlon XP (Thoroughbred)"},
    {IA32CPUVendor::AMD, 6, 10, "AMD Athlon XP (Barton)"},

    // Cyrix / VIA
    {IA32CPUVendor::Cyrix, 5, 2, "Cyrix 6x86 (M1)"},
    {IA32CPUVendor::Cyrix, 5, 4, "Cyrix MediaGX"},
    {IA32CPUVendor::Cyrix, 6, 0, "Cyrix 6x86MX (M2)"},
    {IA32CPUVendor::VIA, 6, 6, "VIA Cyrix III (Samuel)"},
    {IA32CPUVendor::VIA, 6, 7, "VIA C3 (Samuel 2/Ezra)"},
    {IA32CPUVendor::VIA, 6, 9, "VIA C3 (Nehemiah)"},
  };

  void ToDeviceDisplayName(
    IA32CPUVendor vendor,
    UInt32 family,
    UInt32 model,
    char* buffer,
    Size bufferSize
  ) {
    for (const FamilyModelName& entry : KnownCPUs) {
      if (
        entry.Vendor == vendor &&
        entry.Family == family &&
        entry.Model == model
      ) {
        CString::Copy(entry.Name, buffer, bufferSize);

        return;
      }
    }

    CString::Format(
      buffer,
      bufferSize,
      "%s Family %u Model %u",
      ToShortName(vendor),
      family,
      model
    );
  }
}
