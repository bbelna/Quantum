/**
 * @file Kernel/Arch/IA32/Memory/ToIA32PageTableFlags.hpp
 * @brief Declares and implements
 *        @ref @QKrnlIA32::Memory::ToIA32PageTableFlags.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Enum.hpp>

#include <KernelTypes.hpp>
#include <Memory/MemoryMappingPermissions.hpp>

#include "IA32PageTableFlags.hpp"

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief Converts @ref MemoryMappingPermissions to @ref IA32PageTableFlags.
   * @param permissions The @ref MemoryMappingPermissions to convert.
   * @return The corresponding @ref IA32PageTableFlags.
   */
  IA32PageTableFlags ToIA32PageTableFlags(MemoryMappingPermissions permissions) {
    IA32PageTableFlags flags = IA32PageTableFlags::None;

    if (
      Enum::HasFlag(
        permissions,
        MemoryMappingPermissions::Read
      )
    ) {
      flags |= IA32PageTableFlags::Present;
    }

    if (
      Enum::HasFlag(
        permissions,
        MemoryMappingPermissions::Write
      )
    ) {
      flags |= IA32PageTableFlags::Write;
    }

    if (
      Enum::HasFlag(
        permissions,
        MemoryMappingPermissions::User
      )
    ) {
      flags |= IA32PageTableFlags::User;
    }

    return flags;
  }
}
