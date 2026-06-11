/**
 * @file Kernel/Arch/IA32/UserMode/IA32UserModeTrampoline.hpp
 * @brief Declares and implements
 *        @ref @QKrnlIA32::UserMode::IA32UserModeTrampoline.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <KernelContext.hpp>
#include <KernelLog.hpp>

#include "IA32UserModeManager.hpp"

namespace Quantum::Kernel::Arch::IA32::UserMode {
  /**
   * @brief Trampoline for user mode @ref Thread entry.
   *
   * This function is called when a user mode @ref Thread is first scheduled.
   * The @ref Thread @ref Thread::Context has `EAX` and `EBX` values equal to
   * the user `EIP` and user `ESP`, respectively. We retrieve these values and
   * call @ref IA32UserModeManager::Enter to transition to user mode.
   */
  [[noreturn]]
  inline void UserModeTrampoline() {
    // retrieve user mode entry info from registers FIRST, before any
    // compiler-generated code can clobber them; the scheduler restores
    // EAX = userEIP, EBX = userESP from the thread context
    UInt32 userEIP, userESP;

    asm volatile(
      "mov %%eax, %0\n"
      "mov %%ebx, %1\n"
      : "=r"(userEIP), "=r"(userESP)
    );

    KernelContext* context = Context;

    KLOG_DEBUG(
      "User mode trampoline EIP=%p ESP=%p",
      userEIP,
      userESP
    );

    // build entry info and transition to user mode
    UserModeEntryInfo entryInfo {
      .EntryPoint = userEIP,
      .StackPointer = userESP
    };

    context->UserModeManager->Enter(entryInfo);

    // unreachable
    __builtin_unreachable();
  }
}
