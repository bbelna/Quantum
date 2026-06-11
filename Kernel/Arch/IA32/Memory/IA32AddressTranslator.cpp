/**
 * @file Kernel/Arch/IA32/Memory/IA32AddressTranslator.cpp
 * @brief Implements @ref @QKrnlIA32::Memory::IA32AddressTranslator.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Arch/IA32/Drivers/CPU/IA32CPUDriver.hpp>

#include "IA32AddressTranslator.hpp"

namespace Quantum::Kernel::Arch::IA32::Memory {
  UIntPtr IA32AddressTranslator::VirtualToPhysical(UIntPtr virtualAddress) {
    return _cpu->VirtualToPhysical(virtualAddress);
  }

  UIntPtr IA32AddressTranslator::PhysicalToVirtual(UIntPtr physicalAddress) {
    return _cpu->PhysicalToVirtual(physicalAddress);
  }
}
