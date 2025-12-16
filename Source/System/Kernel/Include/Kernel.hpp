/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Kernel.hpp
 * Core kernel implementation.
 */

#pragma once

#include <Types/Primitives.hpp>
#include <Types/String.hpp>

#define PANIC(msg) ::Quantum::System::Kernel::Kernel::Panic((msg), __FILE__, __LINE__, __FUNCTION__)

namespace Quantum::System::Kernel {
  /**
   * Core kernel implementation.
   */
  class Kernel {
    public:
      using String = Types::String;

      /**
       * Initializes the kernel.
       * @param bootInfoPhysicalAddress
       *   Physical address of the boot info block.
       */
      static void Initialize(UInt32 bootInfoPhysicalAddress);

      /**
       * Traces the kernel version and copyright information to the console.
       */
      static void TraceVersionAndCopyright(); 

      /**
       * Panic the kernel with a message. Use the macro `PANIC(msg)` to include
       * file, line, and function information automatically.
       * @param message
       *   Panic message.
       * @param file
       *   The source file where the panic occurred (optional).
       * @param line
       *   The line number where the panic occurred (optional).
       * @param function
       *   The function name where the panic occurred (optional).
       */
      static void Panic(
        String message,
        String file = nullptr,
        UInt32 line = -1,
        String function = nullptr
      );
  };
}
