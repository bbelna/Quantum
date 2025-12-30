/**
 * @file System/Kernel/Include/Objects/KernelObjectType.hpp
 * @brief Kernel object type identifiers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Objects {
  /**
   * Kernel object type identifiers.
   */
  enum class KernelObjectType : UInt32 {
    None = 0,
    IPCPort = 1,
    BlockDevice = 2,
    InputDevice = 3,
    IRQLine = 4
  };
}
