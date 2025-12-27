/**
 * @file System/Kernel/Include/Arch/IA32/UserMode.hpp
 * @brief IA32 User mode entry and stack mapping.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 helpers for entering user mode.
   */
  class UserMode {
    public:
      /**
       * Enters ring3 at the given entry point with the provided user stack.
       * @param entryPoint
       *   User-mode entry point address.
       * @param userStackTop
       *   Top of the user-mode stack.
       */
      [[noreturn]] static void Enter(
        UInt32 entryPoint,
        UInt32 userStackTop
      );

      /**
       * Maps a user stack range with user permissions.
       * @param userStackTop
       *   Top of the stack virtual range.
       * @param sizeBytes
       *   Total stack size in bytes.
       * @return
       *   True if the stack was mapped; false on invalid input.
       */
      static bool MapUserStack(UInt32 userStackTop, UInt32 sizeBytes);

    private:
      /**
       * IA32 page size in bytes.
       */
      static constexpr UInt32 _pageSize = 4096;
  };
}
