//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Interrupts.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Declaration of the kernel Interrupts class.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>

namespace Quantum::Kernel {
  struct InterruptContext;

  using InterruptHandler = void(*)(InterruptContext& ctx);

  class Interrupts {
    public:
      /**
       * Initializes the kernel interrupt subsystem.
       */
      static void Initialize();

      /**
       * Registers an interrupt handler for the given vector.
       */
      static void RegisterHandler(UInt8 vector, InterruptHandler handler);
  };
}
