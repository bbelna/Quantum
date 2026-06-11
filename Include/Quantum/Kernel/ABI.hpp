/**
 * @file Include/Quantum/Kernel/ABI.hpp
 * @brief Includes all headers for, and declares, @ref @QKrnl::ABI.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Kernel/KernelOperationPayload.hpp>
#include <Quantum/Kernel/KernelOperationResult.hpp>
#include <Quantum/Structures/List.hpp>

#include "ABI/ABI.hpp"
#include "ABI/IPC.hpp"
#include "ABI/Interrupt.hpp"
#include "ABI/Log.hpp"
#include "ABI/Memory.hpp"
#include "ABI/PortIO.hpp"
#include "ABI/Process.hpp"
#include "ABI/Thread.hpp"

#include "KernelVersion.hpp"
#include "Memory/MemoryPressureInfo.hpp"
#include "Memory/MemoryPressureState.hpp"
#include "Memory/MemorySummary.hpp"
#include "Memory/MemoryTagStat.hpp"

namespace Quantum::Kernel::ABI {
  constexpr UInt32 KernelABIVersion = KernelVersion;

  using KernelPressureState = Memory::MemoryPressureState;
  using KernelMemoryInfo = Memory::MemorySummary;
  using KernelMemoryPressureInfo = Memory::MemoryPressureInfo;
  using KernelMemoryTagStats  = Memory::MemoryTagStat;
}
