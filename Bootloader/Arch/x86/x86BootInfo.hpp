/**
 * @file Bootloader/Arch/x86/x86BootInfo.hpp
 * @brief Declares @ref @QBtldr::Arch::x86::x86BootInfo.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Arch/x86/x86Types.hpp>
#include "E820Region.hpp"

namespace Quantum::Bootloader::Arch::x86 {
  /**
   * @brief Boot information block passed from the bootloader to the
   *        kernel.
   *
   * Resides at physical address `0x8000`. The bootloader populates all
   * fields before jumping to the kernel entry point, which receives the
   * physical address of this structure in `ESI`.
   *
   * Extends @ref IBootInfo so platform-agnostic code can hold an opaque
   * @ref IBootInfo pointer while concrete PC code downcasts to access fields.
   */
  struct x86BootInfo : public IBootInfo {
    /**
     * @brief Number of valid E820 memory map entries in @ref Entries.
     */
    UInt32 EntryCount;

    /**
     * @brief Boot drive tag.
     *
     * Set to `0x424F0000 | bootDrive`, where `bootDrive` is the BIOS drive
     * number that stage 2 stored here before entering protected mode.
     */
    UInt32 Reserved;

    /**
     * @brief Physical address where the initial image was loaded.
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
     *
     * Read from the binary header at the start of the initial image.
     */
    UInt32 InitialProcessEntryPointOffset;

    /**
     * @brief Size of the initial process binary in bytes.
     *
     * Read from the binary header at the start of the initial image. The
     * region from the image base to `InitialProcessImageSize` is the
     * process binary; everything after it is image data passed to the
     * process.
     */
    UInt32 InitialProcessImageSize;

    /**
     * @brief Virtual address where the initial process is mapped.
     *
     * Set by the platform initializer during kernel handoff preparation.
     */
    UInt32 InitialProcessAddress;

    /**
     * @brief Name of the initial process (null-terminated, max 32 bytes).
     */
    char InitialProcessName[32];

    /**
     * @brief Graphics capability flags.
     *
     * - Bit 0: graphics mode is active.
     *
     * - Bit 1: linear framebuffer is available.
     */
    UInt32 GraphicsFlags;

    /**
     * @brief Framebuffer width in pixels.
     */
    UInt32 GraphicsWidth;

    /**
     * @brief Framebuffer height in pixels.
     */
    UInt32 GraphicsHeight;

    /**
     * @brief Bytes per scanline (pitch).
     */
    UInt32 GraphicsPitch;

    /**
     * @brief Bits per pixel (color depth).
     */
    UInt32 GraphicsBPP;

    /**
     * @brief Physical address of the linear framebuffer.
     */
    UInt32 GraphicsFramebuffer;

    /**
     * @brief Total size of the framebuffer in bytes.
     */
    UInt32 GraphicsFramebufferSize;

    /**
     * @brief E820 memory map entries.
     */
    E820Region Entries[32];
  };
}
