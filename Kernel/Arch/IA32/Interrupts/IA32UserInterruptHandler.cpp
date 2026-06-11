/**
 * @file Kernel/Interrupts/UserInterruptHandler.cpp
 * @brief Implements @ref @QKrnlIA32::Interrupts::UserInterruptHandler.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Concurrency/ThreadManager.hpp>
#include <Drivers/Interrupts/IInterruptControllerDriver.hpp>
#include <KernelContext.hpp>
#include <Resources/KernelResourceManager.hpp>

#include "IA32InterruptContext.hpp"
#include "IA32UserInterruptHandler.hpp"

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  KernelContext* IA32UserInterruptHandler::_context = nullptr;

  void IA32UserInterruptHandler::Initialize(KernelContext* context) {
    _context = context;
  }

  IInterruptContext* IA32UserInterruptHandler::Handle(
    IInterruptContext& context
  ) {
    IA32InterruptContext& ia32Context = static_cast<IA32InterruptContext&>(
      context
    );
    UInt8 irq
      = static_cast<UInt8>(ia32Context.Vector)
      - _context->InterruptController->GetBaseVector();

    Result<IA32InterruptResource*> result
      = _context->KernelResourceManager->FindByObjectID<IA32InterruptResource>(
          KernelResourceType::Interrupt,
          irq
        );

    if (!result.Success) return nullptr;

    IA32InterruptResource* resource = result.Data;

    if (resource->WaitingThread) {
      _context->ThreadManager->Resume(resource->WaitingThread);

      resource->WaitingThread = nullptr;
    } else {
      resource->PendingCount++;
    }

    return nullptr;
  }
}
