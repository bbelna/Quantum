/**
 * @file Kernel/Concurrency/IThreadContextManager.hpp
 * @brief Declares @ref @QKrnl::Concurrency::IThreadContextManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Abstract interface for managing the architecture-specific
   *        @ref Thread @ref Thread::Context.
   *
   * Implementations must provide the low-level @ref Thread
   * @ref Thread::Context setup and switching operations for the target
   * architecture.
   */
  class IThreadContextManager {
    public:
      virtual ~IThreadContextManager() = default;

      /**
       * @brief
       *   Initializes a @ref Thread @ref Thread::Context.
       * @param thread
       *   Pointer to the @ref Thread whose @ref Thread::Context will be
       *   set up.
       * @param entryPoint
       *   The address of the entry point function.
       * @param argument
       *   The argument value placed in the ABI-defined register/stack slot.
       * @param stackTop
       *   The top (highest address) of the @ref Thread @ref Stack.
       * @param isKernelThread
       *   `true` if segment selectors should be set to ring-0; `false` for
       *   ring-3 (user mode).
       * @return
       *   `true` on success; `false` on failure.
       */
      virtual bool InitializeContext(
        Thread* thread,
        UIntPtr entryPoint,
        UIntPtr argument,
        UIntPtr stackTop,
        bool isKernelThread
      ) = 0;

      /**
       * @brief Initializes a @ref Thread with an empty kernel-mode
       *        @ref Thread::Context.
       * @param thread The @ref Thread to initialize.
       * @param stackTop The top (highest address) of the @ref Thread
       *                 @ref Kernel @ref Stack.
       * @return `true` on success; `false` on failure.
       *
       * Creates a zeroed @ref IInterruptContext on the kernel stack with
       * kernel segment selectors. Used for user-mode @ref Thread instances
       * whose @ref Thread::Context is subsequently configured by
       * @ref IUserModeManager::PrepareThread.
       */
      virtual bool InitializeEmptyContext(
        Thread* thread,
        UIntPtr stackTop
      ) = 0;

      /**
       * @brief
       *   Performs a @ref Thread::Context switch from one @ref Thread to
       *   another.
       * @param fromContext
       *   Pointer to the current @ref Thread @ref IInterruptContext.
       * @param toThread
       *   Pointer to the @ref Thread to switch to; its @ref Thread::Context
       *   must have been previously initialized.
       * @return
       *   @ref IInterruptContext pointer for @p toThread, ready to be
       *   returned to the interrupt dispatcher.
       */
      virtual IInterruptContext* SwitchContext(
        IInterruptContext* fromContext,
        Thread* toThread
      ) = 0;

      /**
       * @brief
       *   Clones a @ref Thread @ref Thread::Context into another
       *   @ref Thread.
       * @param source
       *   Pointer to the source @ref Thread whose @ref Thread::Context is
       *   copied.
       * @param dest
       *   Pointer to the destination @ref Thread to receive the cloned
       *   @ref Thread::Context.
       * @return
       *   `true` on success; `false` on failure.
       * @note
       *   The caller is responsible for adjusting the child's return value
       *   (via @ref SetReturnValue) and stack pointer after cloning.
       */
      virtual bool CloneContext(
        const Thread* source,
        Thread* destination
      ) = 0;

      /**
       * @brief
       *   Gets the instruction pointer from an @ref IInterruptContext.
       * @param context
       *   Pointer to the @ref IInterruptContext to get the instruction pointer
       *   from.
       * @return 
       *   The instruction pointer value.
       */
      virtual UIntPtr GetInstructionPointer(
        const IInterruptContext* context
      ) const = 0;

      /**
       * @brief Gets the stack pointer from an @ref IInterruptContext.
       * @param context Pointer to the @ref IInterruptContext to get the
       *                stack pointer from.
       * @return The stack pointer value.
       */
      virtual UIntPtr GetStackPointer(
        const IInterruptContext* context
      ) const = 0;

      /**
       * @brief Sets the return value in an @ref IInterruptContext.
       * @param context Pointer to the @ref IInterruptContext to set the
       *                return value for.
       * @param value The return value to set.
       */
      virtual void SetReturnValue(
        IInterruptContext* context,
        UInt32 value
      ) = 0;
  };
}
