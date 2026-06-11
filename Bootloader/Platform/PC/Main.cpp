/**
 * @file Bootloader/Platform/PC/Main.cpp
 * @brief Implements @ref @QBtldr::Platform::PC::Main for the PC platform.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "PCPlatformInitializer.hpp"

namespace Quantum::Bootloader::Platform::PC {
  /**
   * @brief PC platform entry point.
   * @param bootInfoAddress Physical address of @ref x86BootInfo.
   *
   * Called by the stage 2 assembly stub.
   *
   * Casts the raw address to a concrete @ref x86BootInfo pointer first, then
   * wires up all arch/platform services into a @ref BootloaderContext and
   * hands off to @ref Bootloader @ref Bootloader::Run.
   */
  extern "C" [[noreturn]] void Main(UInt32 bootInfoAddress) {
    // cast to concrete type before constructing any other objects so that
    // all downstream code receives either IBootInfo* or x86BootInfo*
    x86BootInfo* info = reinterpret_cast<x86BootInfo*>(bootInfoAddress);

    static PCPlatformInitializer platformInitializer;
    static x86CPUDriver cpu;
    static x86KernelLauncher kernelLauncher;
    static BootloaderContext context = {};

    context.PlatformInitializer = &platformInitializer;
    context.CPU = &cpu;
    context.KernelLauncher = &kernelLauncher;

    static Bootloader bootloader;

    bootloader.Run(info, &context);
  }
}
