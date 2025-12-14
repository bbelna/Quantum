//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/LinkerSymbols.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Linker-provided symbols for the IA32 kernel image.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>

extern "C" {
  extern UInt8 __phys_start;
  extern UInt8 __phys_end;

  extern UInt8 __bss_start;
  extern UInt8 __bss_end;
  extern UInt8 __phys_bss_start;
  extern UInt8 __phys_bss_end;
  extern UInt8 __virt_start;
  extern UInt8 __virt_end;

  extern UInt8 __hh_virt_start;
  extern UInt8 __hh_phys_start;
}
