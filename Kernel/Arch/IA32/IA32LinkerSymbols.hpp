/**
 * @file Kernel/Arch/IA32/IA32LinkerSymbols.hpp
 * @brief Declares @ref @QKrnlIA32 linker symbols.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

/**
 * @brief Virtual (link-time) base address of the kernel image.
 */
#define KERNEL_VIRTUAL_BASE reinterpret_cast<UInt32>(&__virt_start)

/**
 * @brief Physical (load-time) base address of the kernel image.
 */
#define KERNEL_PHYSICAL_BASE reinterpret_cast<UInt32>(&__phys_start)

/**
 * @brief Physical (load-time) end address of the kernel image.
 */
#define KERNEL_PHYSICAL_END reinterpret_cast<UInt32>(&__phys_end)

/**
 * @brief Virtual (link-time) base address of the kernel higher-half image.
 */
#define KERNEL_HIGHER_HALF_VIRTUAL_BASE \
  reinterpret_cast<UInt32>(&__hh_virt_start)

/**
 * @brief Physical (load-time) base address of the kernel higher-half image.
 */
#define KERNEL_HIGHER_HALF_PHYSICAL_BASE \
  reinterpret_cast<UInt32>(&__hh_phys_start)

extern "C" {
  /**
   * @brief Start of the kernel's physical (load-time) memory region.
   */
  extern UInt8 __phys_start;

  /**
   * @brief End of the kernel's physical (load-time) memory region.
   */
  extern UInt8 __phys_end;

  /**
   * @brief Start of the kernel's virtual BSS segment.
   */
  extern UInt8 __bss_start;

  /**
   * @brief End of the kernel's virtual BSS segment.
   */
  extern UInt8 __bss_end;

  /**
   * @brief Start of the kernel's physical (load-time) BSS segment.
   */
  extern UInt8 __phys_bss_start;

  /**
   * @brief End of the kernel's physical (load-time) BSS segment.
   */
  extern UInt8 __phys_bss_end;

  /**
   * @brief Start of the kernel's virtual (link-time) memory region.
   */
  extern UInt8 __virt_start;

  /**
   * @brief End of the kernel's virtual (link-time) memory region.
   */
  extern UInt8 __virt_end;

  /**
   * @brief Start of the higher-half virtual memory region occupied by the
   *        kernel.
   */
  extern UInt8 __hh_virt_start;

  /**
   * @brief Start of the higher-half physical (load-time) memory region
   *        occupied by the kernel.
   */
  extern UInt8 __hh_phys_start;
}
