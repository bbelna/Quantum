/**
 * @file System/Kernel/Include/Main.hpp
 * @brief Kernel main entry point.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel {
  /**
   * Main kernel entry point.
   * @param bootInfoPhysicalAddress
   *   Physical address of the boot info block.
   */
  void Main(UInt32 bootInfoPhysicalAddress);
}
