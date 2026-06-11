/**
 * @file Kernel/Arch/IA32/Memory/IA32AddressTranslator.hpp
 * @brief Declares @ref @QKrnlIA32::Memory::IA32AddressTranslator.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Arch/IA32/Drivers/IA32DriverTypes.hpp>
#include <KernelTypes.hpp>
#include <Memory/IAddressTranslator.hpp>

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief IA-32 implementation of @ref IAddressTranslator.
   *
   * Delegates to the @ref IA32CPUDriver for virtual-to-physical and
   * physical-to-virtual address translation.
   */
  class IA32AddressTranslator : public IAddressTranslator {
    public:
      /**
       * @brief Initializes the @ref IA32AddressTranslator.
       * @param cpu Pointer to the @ref IA32CPUDriver.
       */
      void Initialize(IA32CPUDriver* cpu) {
        _cpu = cpu;
      }

      /**
       * @brief Translates a virtual address to a physical address.
       * @param virtualAddress The virtual address to translate.
       * @return The corresponding physical address.
       */
      UIntPtr VirtualToPhysical(UIntPtr virtualAddress) override;

      /**
       * @brief Translates a physical address to a virtual address.
       * @param physicalAddress The physical address to translate.
       * @return The corresponding virtual address.
       */
      UIntPtr PhysicalToVirtual(UIntPtr physicalAddress) override;

    private:
      /**
       * @brief Pointer to the @ref IA32CPUDriver.
       */
      IA32CPUDriver* _cpu = nullptr;
  };
}
