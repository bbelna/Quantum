/**
 * @file Include/Quantum/Core.hpp
 * @brief Includes all headers for, and declares, @ref @QCore.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Core/Address.hpp"
#include "Core/Align.hpp"
#include "Core/Byte.hpp"
#include "Core/Cast.hpp"
#include "Core/Color.hpp"
#include "Core/ConfigurationParser.hpp"
#include "Core/CRC32.hpp"
#include "Core/CString.hpp"
#include "Core/Definitions.hpp"
#include "Core/Enum.hpp"
#include "Core/IDAllocator.hpp"
#include "Core/IInitializer.hpp"
#include "Core/IProvider.hpp"
#include "Core/Logging.hpp"
#include "Core/Math.hpp"
#include "Core/Resource.hpp"
#include "Core/Result.hpp"
#include "Core/Singleton.hpp"
#include "Core/Sort.hpp"
#include "Core/Types.hpp"
#include "Core/Value.hpp"

/**
 * @brief Quantum's core framework.
 * 
 * Includes fundamental types, utilities, and base classes used throughout the
 * Quantum codebase.
 * 
 * This namespace is intended to be a dependency-free foundation that can be
 * used by all other modules without introducing circular dependencies.
 * It does NOT contain any platform-specific code or OS abstractions.
 */
namespace Quantum::Core {}
