/**
 * @file Kernel/Arch/IA32/Concurrency/IA32ThreadContextManager.hpp
 * @brief Declares @ref @QKrnlIA32::Concurrency::ThreadContextManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Arch/IA32/Drivers/IA32DriverTypes.hpp>
#include <Concurrency/IThreadContextManager.hpp>

#include "IA32ConcurrencyConstants.hpp"
#include "IA32TaskStateSegmentManager.hpp"

namespace Quantum::Kernel::Arch::IA32::Concurrency {
  /**
   * @brief IA-32 implementation of @ref IThreadContextManager.
   *
   * Handles @ref Thread @ref Thread::Context initialization, switching, and
   * cloning for IA-32 @ref Thread. New @ref Thread get an
   * @ref IA32InterruptContext pushed onto their kernel stack so that the first
   * `iret` transfers control to the entry point. Context switches swap the
   * saved `ESP`, update the TSS `ESP0` for the incoming thread, and (for user
   * @ref Thread) reload the @ref IA32PageDirectory via `CR3`.
   */
  class IA32ThreadContextManager : public IThreadContextManager {
    public:
      /**
       * @brief Creates a new @ref IA32ThreadContextManager.
       * @param cpu Pointer to the @ref IA32CPUDriver.
       * @param tssManager Pointer to the @ref IA32TaskStateSegmentManager.
       * @param kernelAddressSpace Pointer to the kernel's @ref IAddressSpace.
       * @param maydayHandler Pointer to a concrete implementation of
       *                      @ref IMaydayHandler.
       */
      IA32ThreadContextManager(
        IA32CPUDriver* cpu,
        IA32TaskStateSegmentManager* tssManager,
        IAddressSpace* kernelAddressSpace,
        IMaydayHandler* maydayHandler
      );

      /**
       * @brief Destroys the @ref IA32ThreadContextManager.
       */
      ~IA32ThreadContextManager() override = default;

      /**
       * @brief Initializes a thread's context for first execution.
       * @param thread The thread to initialize.
       * @param entryPoint The entry point function address.
       * @param argument The argument to pass to the entry point.
       * @param stackTop The top of the thread's stack.
       * @param isKernelThread `true` if this is a kernel thread; `false` for user.
       * @return `true` on success; `false` on failure.
       */
      bool InitializeContext(
        Thread* thread,
        UIntPtr entryPoint,
        UIntPtr argument,
        UIntPtr stackTop,
        bool isKernelThread
      ) override;

      /**
       * @brief Initializes a thread with an empty kernel-mode context.
       * @param thread The thread to initialize.
       * @param stackTop The top of the thread's kernel stack.
       * @return `true` on success; `false` on failure.
       */
      bool InitializeEmptyContext(
        Thread* thread,
        UIntPtr stackTop
      ) override;

      /**
       * @brief Performs a context switch from one thread to another.
       * @param fromContext The current thread's saved context.
       * @param toThread The thread to switch to.
       * @return The context pointer for the new thread.
       */
      IInterruptContext* SwitchContext(
        IInterruptContext* fromContext,
        Thread* toThread
      ) override;

      /**
       * @brief Clones a thread's context for fork-like operations.
       * @param source The source thread context to clone.
       * @param dest The destination thread to receive the cloned context.
       * @return `true` on success; `false` on failure.
       */
      bool CloneContext(
        const Thread* source,
        Thread* dest
      ) override;

      /**
       * @brief Gets the instruction pointer from a saved context.
       * @param context The saved interrupt context.
       * @return The instruction pointer value.
       */
      UIntPtr GetInstructionPointer(
        const IInterruptContext* context
      ) const override;

      /**
       * @brief Gets the stack pointer from a saved context.
       * @param context The saved interrupt context.
       * @return The stack pointer value.
       */
      UIntPtr GetStackPointer(
        const IInterruptContext* context
      ) const override;

      /**
       * @brief Sets the return value in a saved context (EAX register).
       * @param context The saved interrupt context.
       * @param value The return value to set.
       */
      void SetReturnValue(
        IInterruptContext* context,
        UInt32 value
      ) override;

      /**
       * @brief Gets the TSS manager.
       * @return The TSS manager pointer.
       */
      IA32TaskStateSegmentManager* GetTSSManager() const;

    private:
      /**
       * @brief The IA-32 CPU driver.
       */
      IA32CPUDriver* _cpu = nullptr;

      /**
       * @brief The TSS manager for kernel stack switching.
       */
      IA32TaskStateSegmentManager* _tssManager = nullptr;

      /**
       * @brief The kernel's address space.
       */
      IAddressSpace* _kernelAddressSpace = nullptr;

      /**
       * @brief Pointer to the mayday handler.
       */
      IMaydayHandler* _maydayHandler = nullptr;
  };
}
