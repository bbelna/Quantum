/**
 * @file Kernel/Arch/IA32/UserMode/IA32UserModeManager.hpp
 * @brief Declares @ref @QKrnlIA32::UserMode::IA32UserModeManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Arch/IA32/Concurrency/IA32ConcurrencyTypes.hpp>
#include <KernelTypes.hpp>
#include <UserMode/IUserModeManager.hpp>

#include "IA32UserModeConstants.hpp"

namespace Quantum::Kernel::Arch::IA32::UserMode {
  /**
   * @brief IA-32 implementation of @ref IUserModeManager.
   *
   * Handles the transition between kernel mode (ring 0) and user mode
   * (ring 3) on IA-32 processors. Uses `iret` to enter user mode and
   * relies on the TSS for stack switching on interrupts.
   */
  class IA32UserModeManager : public IUserModeManager {
    public:
      /**
       * @brief Creates a @ref IA32UserModeManager.
       * @param tssManager Pointer to the @ref IA32TaskStateSegmentManager.
       */
      explicit IA32UserModeManager(
        IA32TaskStateSegmentManager* tssManager
      );

      /**
       * @brief Destroys the @ref IA32UserModeManager.
       */
      ~IA32UserModeManager() override = default;

      /**
       * @brief Prepares a @ref Thread for user mode execution.
       * @param thread The @ref Thread to prepare.
       * @param entryInfo @ref UserModeEntryInfo containing the user mode entry
       *                  point and stack pointer.
       * @return `true` on success; `false` on failure.
       */
      bool PrepareThread(
        Thread* thread,
        const UserModeEntryInfo& entryInfo
      ) override;

      /**
       * @brief Enters user mode for the current thread.
       * @param entryInfo @ref UserModeEntryInfo containing the user mode entry
       *                  point and stack pointer.
       *
       * Pushes the user-mode SS, ESP, EFLAGS, CS, and EIP onto the
       * kernel stack in the order expected by `iret`, then executes
       * `iret` to transition to ring 3. This function does not return.
       */
      [[noreturn]]
      void Enter(const UserModeEntryInfo& entryInfo) override;

      /**
       * @brief Checks if the current execution context is in user mode.
       * @return `true` if currently in user mode; `false` otherwise.
       */
      bool IsUserMode() const override;

      /**
       * @brief Checks if user mode is supported on this architecture.
       * @return `true` (always supported on IA-32).
       */
      bool IsSupported() const override {
        return true;
      }

      /**
       * @brief Gets the user mode code segment selector.
       * @return The user mode code segment selector (`0x1B`).
       */
      UInt16 GetUserCodeSegment() const override {
        return UserCodeSelector;
      }

      /**
       * @brief Gets the user mode data segment selector.
       * @return The user mode data segment selector (`0x23`).
       */
      UInt16 GetUserDataSegment() const override {
        return UserDataSelector;
      }

      /**
       * @brief Gets the user mode stack segment selector.
       * @return The user mode stack segment selector (`0x23`).
       */
      UInt16 GetUserStackSegment() const override {
        return UserStackSelector;
      }

    private:
      /**
       * @brief Pointer to the @ref IA32TaskStateSegmentManager.
       */
      IA32TaskStateSegmentManager* _tssManager = nullptr;
  };
}
