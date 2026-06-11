/**
 * @file Kernel/Arch/IA32/Memory/IA32GDTDescriptor.hpp
 * @brief Declares @ref @QKrnlIA32::Memory::IA32GDTDescriptor.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief Opaque `void` pointer to the IA-32 Global Descriptor Table (GDT)
   *        descriptor.
   */
  extern "C" void* IA32GDTDescriptor;
}
