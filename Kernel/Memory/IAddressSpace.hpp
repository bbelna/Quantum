/**
 * @file Kernel/Memory/IAddressSpace.hpp
 * @brief Declares @ref @QKrnl::Memory::IAddressSpace.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::Kernel::Memory {
  /**
   * @brief Architecture-independent base type for address spaces.
   *
   * Concrete architectures inherit from this type. Core kernel code uses
   * `IAddressSpace*` and `IAddressSpace&` to avoid depending on
   * architecture-specific headers.
   *
   * This is intentionally an empty, non-virtual struct so that it does not
   * alter the layout or alignment of derived types (EBO applies).
   */
  struct IAddressSpace {};
}
