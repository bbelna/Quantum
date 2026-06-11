/**
 * @file Kernel/Arch/IA32/Memory/IA32GDT.hpp
 * @brief Declares @ref @QKrnlIA32::Memory::IA32GDT.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "IA32GDTEntry.hpp"

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief The IA-32 Global Descriptor Table (GDT).
   */
  extern "C" IA32GDTEntry IA32GDT[];
}
