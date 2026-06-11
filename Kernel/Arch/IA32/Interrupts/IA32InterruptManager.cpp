/**
 * @file Kernel/Arch/IA32/Interrupts/IA32InterruptManager.cpp
 * @brief Implements @ref @QKrnlIA32::Interrupts::IA32InterruptManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <KernelContext.hpp>

#include "IA32IDT.hpp"
#include "IA32InterruptManager.hpp"
#include "IA32UserInterruptHandler.hpp"

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  IA32InterruptManager::IA32InterruptManager(
    IInterruptControllerDriver<
      IA32InterruptVector
    >& interruptControllerDriver,
    KernelContext* kernelContext
  ) : _interruptControllerDriver(interruptControllerDriver) {
    _idt = new IA32IDT(&_interruptControllerDriver);
    _dispatcher = new IA32InterruptDispatcher(
      _idt,
      kernelContext
    );

    IA32UserInterruptHandler::Initialize(kernelContext);
  }

  void IA32InterruptManager::SetHandler(
    IA32InterruptVector vector,
    InterruptHandler handler
  ) {
    _idt->SetHandler(vector, handler);
  }

  IA32InterruptContext* IA32InterruptManager::HandleException(
    IInterruptContext* context
  ) {
    return _idt->Dispatch(static_cast<IA32InterruptContext*>(context));
  }
}
