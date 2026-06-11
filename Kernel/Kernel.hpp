/**
 * @file Kernel/Kernel.hpp
 * @brief Declares @ref @QKrnl::Kernel.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Bootstrap/InitialProcessInfo.hpp"
#include "Bootstrap/IKernelInitializer.hpp"
#include "Modules/KernelModuleRepository.hpp"
#include "KernelConstants.hpp"
#include "Interrupts/IInterruptContext.hpp"
#include "KernelContext.hpp"

namespace Quantum::Kernel {
  /**
   * @brief The main @ref Kernel class.
   */
  class Kernel : public Singleton<Kernel> {
    public:
      /**
       * @brief Initializes the @ref Kernel and all subsystems.
       * @param kernelInitializer
       *   Pointer to a concrete implementation of @ref IKernelInitializer that
       *   will set up all necessary platform- and architecture-specific
       *   components and populate the @ref KernelContext.
       * @param initialProcess
       *   An @ref InitialProcessInfo that details the initial @ref Process to
       *   map and run as the first user-mode @ref Process, provided by the
       *   bootloader via the boot info block.
       *
       * Called once at boot by the platform-specific entry point after
       * low-level hardware initialization is complete. Delegates to
       * @p kernelInitializer for platform setup, then initializes all
       * kernel modules via the @ref KernelModuleRepository in
       * dependency-resolved order.
       */
      void Initialize(
        IKernelInitializer* kernelInitializer,
        InitialProcessInfo initialProcess
      );

      /**
       * @brief Handles a tick of the timer.
       * @param context Reference to the current @ref IInterruptContext.
       * @return Pointer to the next @ref IInterruptContext to switch to
       *         (possibly different @ref Thread).
       */
      IInterruptContext* Tick(IInterruptContext& context);

      /**
       * @brief Handles a yield.
       * @param context Reference to the current @ref IInterruptContext.
       * @return Pointer to the next @ref IInterruptContext to switch to
       *         (possibly different @ref Thread).
       * @note Unlike @ref Tick, this does not increment the timer tick count.
       */
      IInterruptContext* Yield(IInterruptContext& context);

    private:
      /**
       * @brief The @ref KernelContext containing pointers to all @ref Kernel
       *        subsystems and managers.
       */
      KernelContext _context;

      /**
       * @brief The kernel module repository.
       */
      Core::KernelModuleRepository _modules;

      /**
       * @brief Registers all kernel modules with the repository.
       */
      void _registerModules();

      /**
       * @brief Checks if the kernel initialization succeeded.
       * @return `true` if kernel initialization succeeded, `false` otherwise.
       */
      bool _didInitializeSuccessfully();

      /**
       * @brief Logs the OS version and build information.
       */
      void _logVersion();

      /**
       * @brief Initializes the initial process.
       * @param initialProcess
       *   Information about the initial process image and entry point as
       *   provided by the bootloader.
       */
      void _initializeInitialProcess(InitialProcessInfo initialProcess);

      /**
       * @brief
       *   Creates the @ref Process and @ref Thread for the initial
       *   @ref Process.
       * @param initialProcess
       *   The @ref InitialProcessInfo containing the entry point offset, image
       *   size, and name.
       * @param imageBaseAddress
       *   The base address of the mapped initial image in the process address
       *   space.
       */
      void _initializeInitialProcessThread(
        InitialProcessInfo initialProcess,
        UIntPtr imageBaseAddress
      );

      /**
       * @brief
       *   Maps the initial image into the kernel's address space.
       * @param imageBlock
       *   The @ref MemoryBlock containing the initial image to map.
       */
      void _mapInitialImage(MemoryBlock imageBlock);

      /**
       * @brief Starts the threading subsystem.
       *
       * Called after all initialization is complete to switch to the
       * idle thread and begin scheduling. This function never returns; control
       * only returns to the kernel through interrupts after this point.
       */
      void _startThreading();
  };
}

namespace QKernel = Quantum::Kernel;

/**
 * @brief Global reference to the @ref @QKrnl::Kernel.
 */
inline QKernel::Kernel& QOSKernel = QKernel::Kernel::Instance();

/**
 * @brief Gets a reference to the @ref @QKrnl::Kernel.
 * @return Reference to the @ref @QKrnl::Kernel.
 */
inline QKernel::Kernel& GetQOSKernel() {
  return QKernel::Kernel::Instance();
}
