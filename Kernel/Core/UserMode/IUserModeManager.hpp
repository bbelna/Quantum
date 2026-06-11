/**
 * @file Kernel/UserMode/IUserModeManager.hpp
 * @brief Declares @ref @QKrnl::UserMode::IUserModeManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

#include "UserModeEntryInfo.hpp"

namespace Quantum::Kernel::UserMode {
  /**
   * @brief Interface for user mode management.
   *
   * Provides architecture-agnostic operations for entering user mode,
   * handling transitions between kernel and user mode, and managing
   * user mode execution contexts.
   */
  class IUserModeManager {
    public:
      virtual ~IUserModeManager() = default;

      /**
       * @brief Prepares a @ref Thread for user mode execution.
       * @param thread The @ref Thread to prepare.
       * @param entryInfo Reference to a @ref UserModeEntryInfo instance.
       * @return `true` on success; `false` on failure.
       *
       * Sets up the @ref Thread @ref Thread::Context so that the next context
       * switch to this @ref Thread will enter user mode at the specified entry
       * point.
       */
      virtual bool PrepareThread(
        Thread* thread,
        const UserModeEntryInfo& entryInfo
      ) = 0;

      /**
       * @brief Enters user mode for the current @ref Thread.
       * @param entryInfo Reference to a @ref UserModeEntryInfo instance.
       * @note This function does not return.
       *
       * It performs the transition from kernel mode to user mode, starting
       * execution at the specified entry point with the specified stack.
       */
      [[noreturn]]
      virtual void Enter(const UserModeEntryInfo& entryInfo) = 0;

      /**
       * @brief Checks if the current execution context is in user mode.
       * @return `true` if currently in user mode; `false` otherwise.
       */
      virtual bool IsUserMode() const = 0;

      /**
       * @brief Checks if user mode is supported on this architecture.
       * @return `true` if user mode is supported; `false` otherwise.
       */
      virtual bool IsSupported() const = 0;

      /**
       * @brief Gets the user mode code segment selector.
       * @return The user mode code segment selector value.
       */
      virtual UInt16 GetUserCodeSegment() const = 0;

      /**
       * @brief Gets the user mode data segment selector.
       * @return The user mode data segment selector value.
       */
      virtual UInt16 GetUserDataSegment() const = 0;

      /**
       * @brief Gets the user mode stack segment selector.
       * @return The user mode stack segment selector value.
       */
      virtual UInt16 GetUserStackSegment() const = 0;
  };
}
