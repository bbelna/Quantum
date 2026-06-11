/**
 * @file Kernel/Arch/IA32/Memory/E820BootInfo.hpp
 * @brief Declares @ref @QKrnlIA32::Memory::E820BootInfo.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "E820Region.hpp"

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief Represents the E820 boot info structure provided by the bootloader.
   *
   * Passed from the bootloader to the kernel at a well-known physical
   * address. Contains the E820 memory map, initial image location and
   * size, initial process metadata, and VESA/VESA graphics mode
   * information discovered during real-mode boot.
   */
  struct E820BootInfo {
    /**
     * @brief Maximum number of boot info entries.
     */
    static constexpr UInt32 MaxEntries = 32;

    /**
     * @brief Converts a physical address to an @ref E820BootInfo pointer.
     * @param physicalAddress
     *   The physical address of the @ref E820BootInfo instance as passed by the
     *   bootloader.
     * @return Pointer to the @ref E820BootInfo at the given address.
     */
    static E820BootInfo* FromPhysicalAddress(UIntPtr physicalAddress) {
      return reinterpret_cast<E820BootInfo*>(physicalAddress);
    }

    /**
     * @brief Number of boot info entries.
     */
    UInt32 EntryCount;

    /**
     * @brief Reserved for future use.
     */
    UInt32 Reserved;

    /**
     * @brief Physical address of the initial image.
     *
     * The initial image contains the initial process binary followed by
     * any image data (e.g., a bundle) that the initial process will
     * consume.
     */
    UInt32 InitialImagePhysicalAddress;

    /**
     * @brief Size of the initial image in bytes.
     */
    UInt32 InitialImageSizeInBytes;

    /**
     * @brief Offset of the initial process entry point from the image base.
     */
    UInt32 InitialProcessEntryPointOffset;

    /**
     * @brief Size of the initial process binary in bytes.
     *
     * The region from the image base to this size is the process binary;
     * everything after it is image data passed to the process.
     */
    UInt32 InitialProcessImageSize;

    /**
     * @brief Virtual address where the initial process is mapped.
     *
     * Set by the bootloader platform initializer during kernel handoff.
     */
    UInt32 InitialProcessAddress;

    /**
     * @brief Name of the initial process (null-terminated, max 32 bytes).
     */
    char InitialProcessName[32];

    /**
     * @brief Graphics capability flags.
     */
    UInt32 GraphicsFlags;

    /**
     * @brief Graphics mode width in pixels.
     */
    UInt32 GraphicsWidth;

    /**
     * @brief Graphics mode height in pixels.
     */
    UInt32 GraphicsHeight;

    /**
     * @brief Graphics mode pitch in bytes per scanline.
     */
    UInt32 GraphicsPitch;

    /**
     * @brief Bits per pixel for the graphics mode.
     */
    UInt32 GraphicsBPP;

    /**
     * @brief Physical base address of the linear framebuffer.
     */
    UInt32 GraphicsFramebuffer;

    /**
     * @brief Framebuffer size in bytes.
     */
    UInt32 GraphicsFramebufferSize;

    /**
     * @brief Boot info entries.
     */
    E820Region Entries[MaxEntries];
  };
}
