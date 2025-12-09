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
       * Panic the kernel with a message.
       * @param message Panic message.
       */
      static void Panic(const char* message);
  };
}
