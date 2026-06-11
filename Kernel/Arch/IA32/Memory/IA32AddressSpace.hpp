/**
 * @file Kernel/Arch/IA32/Memory/IA32AddressSpace.hpp
 * @brief Declares @ref @QKrnlIA32::Memory::AddressSpace.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "IA32PageDirectory.hpp"

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief Type alias for an IA-32 address space.
   *
   * On IA-32, each process address space is represented by a single
   * @ref PageDirectory.
   */
  using IA32AddressSpace = IA32PageDirectory;
}
