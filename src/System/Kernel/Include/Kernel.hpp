//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Kernel.hpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Declaration of the Kernel class.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel {
  class Kernel {
    public:
      /**
       * Initializes the kernel.
       */
      static void Initialize(uint32 bootInfoPhys);

      /**
       * Dumps the kernel version and copyright information to the console.
       */
      static void DumpVersionAndCopyright(); 

      /**
       * Panic the kernel with a message.
       * @param message Panic message.
       * @param file The source file where the panic occurred (optional).
       * @param line The line number where the panic occurred (optional).
       * @param function The function name where the panic occurred (optional).
       */
      static void Panic(
        const char* message,
        const char* file = nullptr,
        int line = -1,
        const char* function = nullptr
      );
  };
}
