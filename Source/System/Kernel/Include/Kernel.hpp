//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Kernel.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Declaration of the Kernel class.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>
#include <Types/String.hpp>

#define PANIC(msg) ::Quantum::Kernel::Kernel::Panic((msg), __FILE__, __LINE__, __FUNCTION__)

namespace Quantum::Kernel {
  using String = Types::String;

  class Kernel {
    public:
      /**
       * Initializes the kernel.
       * @param bootInfoPhysicalAddress Physical address of the boot info block.
       */
      static void Initialize(UInt32 bootInfoPhysicalAddress);

      /**
       * Traces the kernel version and copyright information to the console.
       */
      static void TraceVersionAndCopyright(); 

      /**
       * Panic the kernel with a message. Use the macro `PANIC(msg)` to include
       * file, line, and function information automatically.
       * @param message Panic message.
       * @param file The source file where the panic occurred (optional).
       * @param line The line number where the panic occurred (optional).
       * @param function The function name where the panic occurred (optional).
       */
      static void Panic(
        String message,
        String file = nullptr,
        UInt32 line = -1,
        String function = nullptr
      );
  };
}
