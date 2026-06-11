/**
 * @file Bootloader/Arch/x86.hpp
 * @brief Declares @ref @QBtldr::Arch::x86.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "x86/E820Region.hpp"
#include "x86/x86BootInfo.hpp"
#include "x86/x86CPUDriver.hpp"
#include "x86/x86KernelLauncher.hpp"

/**
 * @brief x86-specific submodule for @ref @QBtldr.
 */
namespace Quantum::Bootloader::Arch::x86 {}
