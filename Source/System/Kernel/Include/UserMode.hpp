/**
 * @file System/Kernel/Include/UserMode.hpp
 * @brief User mode transition handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel {
  class UserMode {
    public:
      /**
       * Maps a user-mode stack at the specified top address.
       * @param userStackTop
       *   Top virtual address of the user stack.
       * @param sizeBytes
       *   Size of the stack in bytes.
       * @return
       *   True on success; false otherwise.
       */
      static bool MapUserStack(UInt32 userStackTop, UInt32 sizeBytes);

      /**
       * Enters user mode at the specified entry point.
       * @param entryPoint
       *   Function pointer to the user-mode entry point.
       * @param userStackTop
       *   Top virtual address of the user stack.
       */
      [[noreturn]] static void Enter(UInt32 entryPoint, UInt32 userStackTop);
  };
}
