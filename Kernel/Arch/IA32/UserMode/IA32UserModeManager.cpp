/**
 * @file Kernel/Arch/IA32/UserMode/IA32UserModeManager.cpp
 * @brief Implements @ref @QKrnlIA32::UserMode::IA32UserModeManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Arch/IA32/Concurrency/IA32ConcurrencyConstants.hpp>
#include <Arch/IA32/Interrupts/IA32InterruptContext.hpp>
#include <Arch/IA32/Interrupts/IA32InterruptTypes.hpp>
#include <Concurrency/Thread.hpp>
#include <KernelLog.hpp>

#include "IA32EnterUserMode.hpp"
#include "IA32UserModeManager.hpp"
#include "IA32UserModeTrampoline.hpp"

namespace Quantum::Kernel::Arch::IA32::UserMode {
  IA32UserModeManager::IA32UserModeManager(
    IA32TaskStateSegmentManager* tssManager
  ) : _tssManager(tssManager) {}

  bool IA32UserModeManager::PrepareThread(
    Thread* thread,
    const UserModeEntryInfo& entryInfo
  ) {
    if (thread != nullptr) {
      IA32InterruptContext* context = static_cast<IA32InterruptContext*>(
        thread->Context
      );

      if (context != nullptr) {
        // for user mode threads, we set up the context to call a kernel-mode
        // trampoline that will then transition to user mode via Enter;
        // the user mode entry info is stored in the thread for the trampoline
        //
        // note: direct iret to user mode from scheduler requires SS/ESP on
        // stack which isn't part of the standard interrupt context for kernel
        // threads; instead, user threads start in kernel mode and call Enter
        // explicitly

        // store entry info for the trampoline (using available registers)
        context->EAX = entryInfo.EntryPoint; // user EIP
        context->EBX = entryInfo.StackPointer; // user ESP

        // set EIP to the kernel trampoline that will call Enter
        context->EIP = reinterpret_cast<UInt32>(&UserModeTrampoline);

        // set up to run in kernel mode initially (trampoline will switch to
        // user)
        context->CS = KernelCodeSegment;
        context->EFLAGS = UserModeEFLAGS;

        // clear other registers
        context->ECX = 0;
        context->EDX = 0;
        context->ESI = 0;
        context->EDI = 0;
        context->EBP = 0;

        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }

  [[noreturn]]
  void IA32UserModeManager::Enter(const UserModeEntryInfo& entryInfo) {
    // transition to user mode using iret
    // stack layout for iret to ring 3:
    //   [ESP+16] SS
    //   [ESP+12] ESP
    //   [ESP+8]  EFLAGS
    //   [ESP+4]  CS
    //   [ESP+0]  EIP
    EnterUserMode(
      UserCodeSelector,
      UserStackSelector,
      entryInfo.EntryPoint,
      entryInfo.StackPointer,
      UserModeEFLAGS
    );

    // unreachable - EnterUserMode never returns
    __builtin_unreachable();
  }

  bool IA32UserModeManager::IsUserMode() const {
    // check the current privilege level by examining CS
    // CPL is in bits 0-1 of CS; if CPL = 3, we're in user mode
    UInt16 cs = 0;

    asm volatile("mov %%cs, %0" : "=r"(cs));

    return (cs & 0x3) == 3;
  }
}
