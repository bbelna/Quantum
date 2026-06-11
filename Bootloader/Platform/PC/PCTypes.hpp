/**
 * @file Bootloader/Platform/PC/PCTypes.hpp
 * @brief Declares core @ref @QBtldr::Platform::PC types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/IInitializer.hpp>
#include <Quantum/Core/Types.hpp>

#include <Arch/x86.hpp>
#include <Bootloader.hpp>
#include <BootloaderContext.hpp>
#include <IPlatformInitializer.hpp>
#include <ISpinner.hpp>
#include <IFileLoader.hpp>

#include "HAL.hpp"

using namespace Quantum::Bootloader;
using namespace Quantum::Bootloader::Arch::x86;
using namespace Quantum::Bootloader::Platform::PC;
using namespace Quantum::Bootloader::Platform::PC::HAL;
