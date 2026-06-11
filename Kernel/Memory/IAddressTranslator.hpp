/**
 * @file Kernel/Memory/IAddressTranslator.hpp
 * @brief Declares @ref @QKrnl::Memory::IAddressTranslator.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Memory {
  /**
   * @brief Abstract interface for translating between virtual and physical
   *        addresses.
   *
   * Architecture-specific implementations (e.g., the IA-32 paging layer)
   * inherit from this interface. The kernel uses @ref IAddressTranslator so
   * that code requiring raw address translation remains
   * architecture-independent.
   */
  class IAddressTranslator {
    public:
      /**
       * @brief Translates a virtual address to its corresponding physical
       *        address.
       * @param virtualAddress The virtual address to translate.
       * @return The corresponding physical address.
       */
      virtual UIntPtr VirtualToPhysical(UIntPtr virtualAddress) = 0;

      /**
       * @brief Translates a physical address to its corresponding virtual
       *        address.
       * @param physicalAddress The physical address to translate.
       * @return The corresponding virtual address.
       */
      virtual UIntPtr PhysicalToVirtual(UIntPtr physicalAddress) = 0;
  };
}
