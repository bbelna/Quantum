/**
 * @file Kernel/Arch/IA32/Bootstrap/IA32BootstrapStage2.cpp
 * @brief Implements @ref @QKrnlIA32::Bootstrap::IA32BootstrapStage2.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#if defined(PLATFORM_PC)
#include <Platform/PC/Bootstrap/PCBootstrapStage3.hpp>
#endif

#include "IA32BootstrapStage2.hpp"

/**
 * @brief Type of global constructor functions.
 */
using InitFunction = void (*)();

extern "C" {
  /**
   * @brief Start marker for the global constructors array.
   *
   * These symbols are defined by the linker script and mark the range of
   * function pointers in the `.init_array` section that need to be called to
   * run global constructors.
   */
  extern InitFunction __init_array_start[];

  /**
   * @brief End marker for the global constructors array.
   *
   * These symbols are defined by the linker script and mark the range of
   * function pointers in the `.init_array` section that need to be called to
   * run global constructors.
   */
  extern InitFunction __init_array_end[];
}

namespace Quantum::Kernel::Arch::IA32::Bootstrap {
  namespace {
    /**
     * @brief SYSENTER entry point defined in `InterruptDescriptorTable.asm`.
     */
    extern "C" void SYSENTER();

    /**
     * @brief Relocates the initial image to its higher-half location.
     * @param bootInfoPhysicalAddress Physical address for @ref E820BootInfo.
     *
     * This function copies the initial image from its initial location to a
     * predefined higher-half address. It also updates @ref E820BootInfo to
     * reflect the new location of the initial image.
     */
    void RelocateInitialImage(UInt32 bootInfoPhysicalAddress) {
      if (bootInfoPhysicalAddress != 0) {
        BootInfo = E820BootInfo::FromPhysicalAddress(bootInfoPhysicalAddress);

        if (
          BootInfo->InitialImagePhysicalAddress != 0 &&
          BootInfo->InitialImageSizeInBytes != 0
        ) {
          UInt32 size = BootInfo->InitialImageSizeInBytes;
          constexpr UInt32 newBase = static_cast<UInt32>(
            IA32InitialImageRelocationBase
          );
          const UInt8* source = reinterpret_cast<const UInt8*>(
            BootInfo->InitialImagePhysicalAddress
          );
          UInt8* destination = reinterpret_cast<UInt8*>(newBase);

          if (BootInfo->InitialImagePhysicalAddress != newBase) {
            for (UInt32 i = 0; i < size; ++i) destination[i] = source[i];

            BootInfo->InitialImagePhysicalAddress = newBase;
          }
        }
      }
    }

    /**
     * @brief Runs global constructors.
     * 
     * This function iterates over the global constructors defined in the
     * `.init_array` section and invokes each one. This ensures that all global
     * objects are properly initialized before the kernel starts executing its
     * main logic.
     */
    void RunGlobalConstructors() {
      for (
        InitFunction* fn = __init_array_start;
        fn < __init_array_end;
        ++fn
      ) if (*fn) (*fn)();
    }
  }

  extern "C" [[noreturn]] void IA32BootstrapStage2(UInt32 bootInfoPhysicalAddress) {
    RelocateInitialImage(bootInfoPhysicalAddress);
    RunGlobalConstructors();

    #if defined(PLATFORM_PC)
    Platform::PC::Bootstrap::PCBootstrapStage3();
    #else
    #error "IA32BootstrapStage2 does not support the current platform"
    #endif
  }
}
