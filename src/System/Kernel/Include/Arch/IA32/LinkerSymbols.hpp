//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/LinkerSymbols.hpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Linker-provided symbols for the IA32 kernel image.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

extern "C" {
  extern uint8 __phys_start;
  extern uint8 __phys_end;

  extern uint8 __bss_start;
  extern uint8 __bss_end;
}
