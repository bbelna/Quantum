/**
 * @file Kernel/Arch/IA32/Concurrency/IA32TaskStateSegmentManager.hpp
 * @brief Declares @ref @QKrnlIA32::Concurrency::TaskStateSegmentManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

#include "IA32TaskStateSegment.hpp"

namespace Quantum::Kernel::Arch::IA32::Concurrency {
  /**
   * @brief GDT selector for the TSS.
   * 
   * The TSS is at GDT index 5 (after null, kernel code, kernel data,
   * user code, user data).
   */
  constexpr UInt16 TSSSelector = 0x28;

  /**
   * @brief Kernel data segment selector.
   */
  constexpr UInt16 KernelDataSelector = 0x10;

  /**
   * @brief Manages the IA-32 Task State Segment (TSS).
   *
   * Owns the TSS structure, writes the TSS descriptor into GDT slot 5
   * (selector 0x28), loads the task register via `ltr`, and provides
   * helpers for updating ESP0. The kernel uses a single TSS shared by
   * all threads; `SetKernelStack` is called on every context switch to
   * point ESP0 at the new thread's kernel stack top so that ring 3 to
   * ring 0 transitions land on the correct stack.
   */
  class IA32TaskStateSegmentManager {
    public:
      /**
       * @brief Constructs and initializes the TSS manager.
       * @param kernelStackTop Initial kernel stack top address.
       */
      explicit IA32TaskStateSegmentManager(
        UIntPtr kernelStackTop
      );

      /**
       * @brief Updates the kernel stack pointer in the TSS.
       * @param stackTop The new kernel stack top for the current thread.
       */
      void SetKernelStack(UIntPtr stackTop);

      /**
       * @brief Gets the current kernel stack pointer from the TSS.
       * @return The current ESP0 value.
       */
      UIntPtr GetKernelStack() const;

    private:
      /**
       * @brief The task state segment owned by this manager.
       */
      IA32TaskStateSegment _tss {};

      /**
       * @brief Initializes the TSS descriptor in the GDT and loads TR.
       */
      void _initializeDescriptor();
  };
}
