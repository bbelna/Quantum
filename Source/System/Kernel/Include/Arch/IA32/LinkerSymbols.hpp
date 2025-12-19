/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/LinkerSymbols.hpp
 * IA32 linker symbols.
 */

#pragma once

#include <Types.hpp>

extern "C" {
  /**
   * Start of the physical memory region occupied by the kernel.
   */
  extern UInt8 __phys_start;

  /**
   * End of the physical memory region occupied by the kernel.
   */
  extern UInt8 __phys_end;

  /**
   * Start of the kernel's BSS segment.
   */
  extern UInt8 __bss_start;

  /**
   * End of the kernel's BSS segment.
   */
  extern UInt8 __bss_end;

  /**
   * Start of the kernel's physical BSS segment.
   */
  extern UInt8 __phys_bss_start;

  /**
   * End of the kernel's physical BSS segment.
   */
  extern UInt8 __phys_bss_end;

  /**
   * Start of the virtual memory region occupied by the kernel.
   */
  extern UInt8 __virt_start;

  /**
   * End of the virtual memory region occupied by the kernel.
   */
  extern UInt8 __virt_end;

  /**
   * Start of the higher-half virtual memory region occupied by the kernel.
   */
  extern UInt8 __hh_virt_start;

  /**
   * End of the higher-half virtual memory region occupied by the kernel.
   */
  extern UInt8 __hh_phys_start;
}
