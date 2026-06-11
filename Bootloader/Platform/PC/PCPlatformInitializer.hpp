/**
 * @file Bootloader/Platform/PC/PCPlatformInitializer.hpp
 * @brief Declares @ref @QBtldr::Platform::PC::PCPlatformInitializer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Platform/PC/PCTypes.hpp>

namespace Quantum::Bootloader::Platform::PC {
  /**
   * @brief PC-specific implementation of @ref IPlatformInitializer.
   */
  class PCPlatformInitializer : public IPlatformInitializer {
    public:
      /**
       * @brief The physical address to load the kernel to.
       */
      static constexpr UInt32 KernelPhysicalAddress = 0x00100000;

      /**
       * @brief The physical address to load the initial image to.
       */
      static constexpr UInt32 InitialImagePhysicalAddress = 0x00200000;

      /**
       * @brief The virtual address where the initial process is mapped.
       */
      static constexpr UInt32 InitialProcessAddress = 0x40000000;

      /**
       * @brief The physical address of the boot info structure.
       */
      static constexpr UInt32 BootInfoPhysicalAddress = 0x8000;

      /**
       * @brief The physical address to load the VESA mode info block to.
       */
      static constexpr UInt32 VESAModeInfoPhysicalAddress = 0x9000;

      /**
       * @brief The physical address to load the VESA info block to.
       */
      static constexpr UInt32 VESAInfoPhysicalAddress = 0x9200;

      /**
       * @brief Performs early platform initialization.
       * @param options Context and boot-info pointers.
       *
       * - Loads the BIOS trampoline GDT via @ref Initialize.
       * - Reads and tags the boot drive from the @ref x86BootInfo
       *   @ref x86BootInfo::Reserved.
       * - Constructs @ref BIOSKeyboardDriver, @ref BIOSTimerDriver, and
       *   @ref BIOSVGADriver; then wires them into the @ref BootloaderContext.
       * - Initializes @ref FAT12FileLoader and attaches a @ref VGASpinner
       *   instance.
       *
       * The @p options.Info pointer is downcast internally to @ref x86BootInfo.
       */
      void Initialize(PlatformInitializationOptions options) override;

      /**
       * @brief Finalizes boot-info for kernel handoff.
       * @param options Context and boot-info pointers.
       *
       * - Writes initial image address, size, initial process entry point
       *   offset, image size, and name into the boot-info block.
       * - Collects the physical memory map via `INT 15h`/E820.
       * - Enumerates VESA modes and sets a
       *   \f$1024\times768\times16\text{ bpp}\f$ linear-framebuffer mode.
       *
       * Must be called after all file loads are complete.
       */
      void PrepareKernelHandoff(PlatformInitializationOptions options) override;

    private:
      /**
       * @brief Zeroes all graphics fields in the boot-info block.
       * @param info Concrete boot-info pointer.
       *
       * Called when VESA mode enumeration fails so the kernel sees a clean
       * "no framebuffer" state.
       */
      void _setGraphicsFailed(IBootInfo* info);

      /**
       * @brief Attempts to set a VESA linear-framebuffer mode.
       * @param mode VESA mode number (without the LFB flag).
       * @return `true` if the BIOS accepted the mode.
       */
      bool _trySetVESAMode(UInt16 mode);

      /**
       * @brief Queries VESA mode information into the scratch buffer.
       * @param mode VESA mode number.
       * @return `true` if the BIOS returned `0x004F` in AX.
       */
      bool _getVESAModeInfo(UInt16 mode);

      /**
       * @brief Reads VESA mode info from the scratch buffer into boot-info.
       * @param info Concrete boot-info pointer.
       */
      void _storeVESAInfo(IBootInfo* info);

      /**
       * @brief Collects the E820 physical memory map into the boot-info block.
       * @param options Context and concrete boot-info pointer.
       */
      void _collectMemoryMap(PlatformInitializationOptions options);

      /**
       * @brief Resets the display and sets a 1024×768×16bpp VESA mode.
       * @param options Context and concrete boot-info pointer.
       */
      void _setupDisplay(PlatformInitializationOptions options);
  };
}
