/**
 * @file Kernel/Platform/PC/Bootstrap/PCKernelInitializer.hpp
 * @brief Declares @ref @QKrnlPC::Bootstrap::PCKernelInitializer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "PCBootstrapTypes.hpp"

namespace Quantum::Kernel::Platform::PC::Bootstrap {
  /**
   * @brief PC platform implementation of @ref IKernelInitializer.
   * @see @ref PCKernelInitializer::Initialize for details on the initialization
   *      steps performed by this class.
   */
  class PCKernelInitializer : public IKernelInitializer {
    public:
      /**
       * @brief Creates a new @ref PCKernelInitializer.
       */
      PCKernelInitializer() = default;

      /**
       * @brief Initializes PC-specific kernel subsystems and populates the
       *        provided @ref KernelContext.
       * @param context Pointer to a @ref KernelContext instance to populate.
       *
       * Called by @ref Kernel::Initialize as the first step of the kernel
       * initialization sequence.
       *
       * On entry, the @ref KernelContext contains only the boot-time
       * @ref KernelContext::LogLevel; on return, every platform- and
       * architecture-dependent field is populated and the machine is ready for
       * the architecture-independent kernel initialization that follows.
       *
       * @ref Initialize performs the following steps, in order:
       *
       *   1. Probes the @ref PCI bus and constructs a @ref PCI driver instance
       *      used by later steps for device discovery.
       *
       *   2. Initializes logging: constructs a @ref SerialCOMDriver on COM1
       *      and a @ref VESADriver, then creates a @ref LogSink for each and
       *      combines them into a @ref SpinlockLog that is installed as
       *      @ref KernelContext::Log.
       *
       *   3. Initializes memory management: through the @ref Arch layer,
       *      constructs and configures the @ref MemoryAllocator from
       *      @ref BootInfo, the @ref PageDirectoryManager, the
       *      @ref MemoryMapper, and the @ref KernelAddressSpaceMap. The
       *      @ref Arch layer also initializes the kernel @ref PageDirectory
       *      (a concrete implementation of @ref IAddressSpace), installs it as
       *      @ref KernelContext::KernelAddressSpace, enables paging, and
       *      enables top-down allocation. After the @ref Arch memory layer
       *      initialization, the @ref HeapAllocator and inserts null-guard and
       *      kernel-region entries into
       *      @ref KernelContext::KernelAddressSpaceMap. Finally, we jump
       *      back to the @ref Arch layer to initialize the Page Attribute
       *      Table (PAT), if supported by the CPU.
       *
       *   4. Maps the graphics framebuffer via @ref VESADriver.
       *      If a supported @ref S3ViRGEDriver GPU is discovered
       *      on the @ref PCI bus, initializes its driver and
       *      redirects the @ref ScreenLogSink to it.
       *
       *   5. Initializes interrupts: constructs
       *      @ref Intel8259Driver (a concrete implementation
       *      of @ref IInterruptControllerDriver), the @ref Arch layer's
       *      concrete implementation of @ref IInterruptManager, and
       *      @ref Intel8253Driver (a concrete implementation of
       *      @ref ITimerDriver). Registers handlers for the timer and
       *      yield IRQ vectors, unmasks the timer IRQ, and enables interrupts.
       *
       *   6. Initializes threading support through the @ref Arch layer:
       *      constructs @ref TaskStateSegmentManager with the current stack
       *      pointer and @ref ThreadContextManager.
       *
       *   7. Initializes, through the @ref Arch layer, @ref UserModeManager
       *      for ring-0-to-ring-3 transitions.
       *
       *   8. Populates @ref KernelContext.PlatformDevices with the active
       *      graphics driver, @ref Intel8253Driver, @ref Intel8259Driver and
       *      @ref SerialCOMDriver.
       */
      void Initialize(KernelContext* context) override;

    private:
      /**
       * @brief Pointer to the @ref SerialCOMDriver used for logging to
       *        serial devices.
       */
      SerialCOMDriver* _serial = nullptr;

      /**
       * @brief Pointer to the @ref VESADriver used for screen output.
       */
      VESADriver* _vesa = nullptr;

      /**
       * @brief Pointer to @ref PCI used for device discovery and configuration.
       */
      PCI* _pci = nullptr;

      /**
       * @brief Pointer to the @ref S3ViRGEDriver, if a supported GPU is
       *        present.
       */
      S3ViRGEDriver* _s3Virge = nullptr;

      /**
       * @brief Pointer to @ref IA32TaskStateSegmentManager used for managing
       *        task state segments and kernel stacks for user mode transitions.
       */
      IA32TaskStateSegmentManager* _tssManager = nullptr;

      /**
       * @brief Screen log sink; redirected to the active graphics driver.
       */
      ScreenLogSink* _screenSink = nullptr;

      /**
       * @brief Static platform device pointer array (room for 6 devices).
       */
      IDriver* _platformDevices[10] = {};

      /**
       * @brief Number of entries in @ref _platformDevices.
       */
      Size _platformDeviceCount = 0;

      /**
       * @brief Initializes the PCI driver.
       * @param context Pointer to a @ref KernelContext instance containing
       *                pointers to all kernel subsystems and managers.
       */
      void _initializePCI(KernelContext* context);

      /**
       * @brief Initializes the kernel log instance and configures log sinks for
       *        the provided context.
       * @param context Pointer to a @ref KernelContext instance containing
       *                pointers to all kernel subsystems and managers.
       */
      void _initializeLog(KernelContext* context);

      void _initializeCPUDriver(KernelContext* context);

      void _initializeHandlers(KernelContext* context);

      /**
       * @brief Initializes memory management subsystems and configures the
       *        kernel context with the appropriate pointers.
       * @param context Pointer to a @ref KernelContext instance containing
       *                pointers to all kernel subsystems and managers.
       */
      void _initializeMemory(KernelContext* context);

      /**
       * @brief Initializes the graphics framebuffer and configures the kernel
       *        context for screen output.
       * @param context Pointer to a @ref KernelContext instance containing
       *                pointers to all kernel subsystems and managers.
       */
      void _initializeScreen(KernelContext* context);

      /**
       * @brief Initializes the interrupt subsystem and configures the kernel
       *        context with the appropriate pointers.
       * @param context Pointer to a @ref KernelContext instance containing
       *                pointers to all kernel subsystems and managers.
       */
      void _initializeInterrupts(KernelContext* context);

      /**
       * @brief Initializes the threading subsystem and configures the kernel
       *        context with the appropriate pointers.
       * @param context Pointer to a @ref KernelContext instance containing
       *                pointers to all kernel subsystems and managers.
       */
      void _initializeThreading(KernelContext* context);

      /**
       * @brief Initializes platform devices and populates the kernel context's
       *        platform device list.
       * @param context Pointer to a @ref KernelContext instance containing
       *                pointers to all kernel subsystems and managers.
       */
      void _initializeUserMode(KernelContext* context);

      /**
       * @brief Initializes platform devices and populates the kernel context's
       *        platform device list.
       * @param context Pointer to a @ref KernelContext instance containing
       *                pointers to all kernel subsystems and managers.
       */
      void _initializeDevices(KernelContext* context);
  };
}
