/**
 * @file Kernel/Arch/IA32/Bootstrap/IA32BootstrapTypes.hpp
 * @brief Declares @QKrnlIA32::Bootstrap types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include <Arch/IA32/Memory/E820BootInfo.hpp>
#include <Arch/IA32/Memory/IA32MemoryTypes.hpp>

namespace Quantum::Kernel::Arch::IA32::Bootstrap {
  [[gnu::section(".start.data")]]
  extern E820BootInfo* BootInfo;

  extern "C"
  [[noreturn]]
  void IA32BootstrapStage2(UInt32 bootInfoPhysicalAddress);
}

using namespace Quantum::Kernel::Arch::IA32::Bootstrap;
