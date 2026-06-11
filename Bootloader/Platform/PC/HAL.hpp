/**
 * @file Bootloader/Platform/PC/HAL.hpp
 * @brief Declares @ref @QBtldr::Platform::PC::HAL.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "HAL/BIOS.hpp"
#include "HAL/BIOSDriver.hpp"
#include "HAL/BIOSKeyboardDriver.hpp"
#include "HAL/BIOSTimerDriver.hpp"
#include "HAL/BIOSVGADriver.hpp"

/**
 * @brief HAL (Hardware Abstraction Layer) submodule for
 *        @ref @QBtldr::Platform::PC.
 */
namespace Quantum::Bootloader::Platform::PC::HAL {}
