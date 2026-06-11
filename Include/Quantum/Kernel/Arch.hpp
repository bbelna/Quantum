/**
 * @file Include/Quantum/Kernel/Arch.hpp
 * @brief Declaration of the IA-32 architecture ABI namespace.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#if defined(ARCH_IA32)
#include "Arch/IA32/ABI.hpp"
#else
#error "Unsupported architecture"
#endif
