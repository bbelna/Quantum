//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Interrupts.hpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Declaration of the kernel Interrupts class.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

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
      static void RegisterHandler(uint8 vector, InterruptHandler handler);
  };
}
