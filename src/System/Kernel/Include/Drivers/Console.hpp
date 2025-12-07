//------------------------------------------------------------------------------
// Quantum
//------------------------------------------------------------------------------
// System/Kernel/Include/Drivers/Console.hpp
// Architecture-agnostic console interface for the kernel.
// Brandon Belna - MIT License
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Drivers {
  class Console {
    public:
      /**
       * Initializes the active console backend.
       */
      static void Initialize();

      /**
       * Writes a single character to the console.
       */
      static void WriteChar(char c);

      /**
       * Writes a null-terminated string to the console.
       */
      static void WriteString(const char* str);
  };
}
