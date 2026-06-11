/**
 * @file Bootloader/BootloaderContext.hpp
 * @brief Declares @ref @QBtldr::BootloaderContext.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "HAL.hpp"
#include "IBootloaderInitializer.hpp"
#include "IFileLoader.hpp"
#include "IPlatformInitializer.hpp"
#include "ISpinner.hpp"

namespace Quantum::Bootloader {
  class IKernelLauncher;

  /**
   * @brief Context structure containing pointers to all bootloader services.
   *
   * Populated by the platform initializer during early init and consumed by
   * the arch-agnostic @ref Bootloader::Run loop.
   */
  struct BootloaderContext {
    /**
     * @brief Optional bootloader-level initializer.
     */
    IBootloaderInitializer* BootloaderInitializer;

    /**
     * @brief Platform initializer providing early-init and handoff.
     */
    IPlatformInitializer* PlatformInitializer;

    /**
     * @brief Kernel launcher that performs the final jump.
     */
    IKernelLauncher* KernelLauncher;

    /**
     * @brief CPU driver.
     */
    ICPUDriver* CPU;

    /**
     * @brief Platform file loader implementation.
     */
    IFileLoader* FileLoader;

    /**
     * @brief Spinner shown during file loads.
     */
    ISpinner* Spinner;

    /**
     * @brief Graphics driver.
     */
    IGraphicsDriver* Graphics;

    /**
     * @brief Keyboard driver.
     */
    IKeyboardDriver* Keyboard;

    /**
     * @brief Timer driver.
     */
    ITimerDriver* Timer;

    /**
     * @brief Physical address to load the kernel image at.
     */
    UIntPtr KernelPhysicalAddress;

    /**
     * @brief Physical address to load the initial image at.
     */
    UIntPtr InitialImagePhysicalAddress;

    /**
     * @brief Size of the loaded initial image in bytes.
     *
     * Written by @ref Bootloader::Run after the initial image is loaded;
     * read by @ref IPlatformInitializer::PrepareKernelHandoff to write
     * into the boot-info block before the kernel jump.
     */
    UInt32 InitialImageSizeInBytes;

    /**
     * @brief Name of the initial process (e.g. `"StartupServer.qbn"`).
     *
     * Set by @ref Bootloader::Run from @ref BootloaderOptions; read by
     * @ref IPlatformInitializer::PrepareKernelHandoff to write into the
     * boot-info block.
     */
    const char* InitialProcessName;

    /**
     * @brief Number of text columns supported by the graphics driver.
     */
    UInt8 TextColumns;

    /**
     * @brief Number of text rows supported by the graphics driver.
     */
    UInt8 TextRows;
  };
}
