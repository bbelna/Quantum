/**
 * @file Kernel/Arch/IA32/Handlers/IA32MaydayHandler.cpp
 * @brief Implements @ref @QKrnlIA32::IA32MaydayHandler.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Concurrency/Thread.hpp>
#include <Concurrency/ThreadManager.hpp>
#include <Concurrency/Process.hpp>
#include <KernelLog.hpp>
#include <Memory/Stack.hpp>

#include "IA32MaydayHandler.hpp"

namespace Quantum::Kernel::Arch::IA32::Handlers {
  IA32MaydayHandler::IA32MaydayHandler(KernelContext* context)
    : _context(context) {}

  [[noreturn]]
  void IA32MaydayHandler::Handle(const char* message) {
    KLOG_CRITICAL(">>> MAYDAY! %s <<<", message);

    Thread* thread
      = _context->ThreadManager
      ? _context->ThreadManager->GetCurrent()
      : nullptr;

    if (thread) {
      Process* process = thread->OwnerProcess;

      KLOG_CRITICAL(
        "  Thread: ID=%u Name=\"%s\" State=%u Priority=%u",
        thread->ID,
        thread->Name,
        static_cast<UInt32>(thread->State),
        Enum::ToBase(thread->BasePriority)
      );

      if (process) {
        KLOG_CRITICAL(
          "  Process: PID=%u Name=\"%s\"",
          process->ID,
          process->Name
        );
      } else {
        KLOG_CRITICAL("  Process: Kernel");
      }

      if (thread->KernelStack) {
        UInt32 stackBase = thread->KernelStack->Base;
        UInt32 stackTop = thread->KernelStack->Top;

        KLOG_CRITICAL(
          "  Kernel stack: %p-%p (%u bytes)",
          stackBase,
          stackTop,
          stackTop - stackBase
        );
      }
    }

    UInt32 currentESP;

    asm volatile("mov %%esp, %0" : "=r"(currentESP));

    KLOG_CRITICAL("  ESP at MAYDAY: ESP=%p", currentESP);

    volatile UInt32* sp = reinterpret_cast<volatile UInt32*>(currentESP);

    KLOG_CRITICAL("  Stack dump (16 dwords from ESP):");

    for (Size i = 0; i < 16; i += 4) {
    KLOG_CRITICAL(
      "    +%p: %p %p %p %p",
      static_cast<UInt32>(i * 4),
        sp[i], sp[i + 1], sp[i + 2], sp[i + 3]
      );
    }

    UInt32 currentEBP;

    asm volatile("mov %%ebp, %0" : "=r"(currentEBP));

    if (thread && thread->KernelStack) {
      UInt32 stackBase = thread->KernelStack->Base;
      UInt32 stackTop  = thread->KernelStack->Top;

      KLOG_CRITICAL("  Call stack (EBP chain):");

      for (
        Size frame = 0;
        frame < 16 && currentEBP != 0;
        ++frame
      ) {
        if (
          currentEBP < stackBase ||
          currentEBP + 8 > stackTop
        ) {
          break;
        }

        UInt32 savedEBP = *reinterpret_cast<UInt32*>(currentEBP);
        UInt32 returnIP = *reinterpret_cast<UInt32*>(currentEBP + 4);

        KLOG_CRITICAL(
          "    #%u  EIP=%p  EBP=%p",
          frame,
          returnIP,
          currentEBP
        );

        currentEBP = savedEBP;
      }
    }

    KLOG_CRITICAL(">>> END OF MAYDAY TRACE <<<");

    asm volatile("cli");

    for (;;) {
      asm volatile("hlt");
    }
  }
}
