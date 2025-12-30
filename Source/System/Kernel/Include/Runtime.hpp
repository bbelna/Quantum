/**
 * @file System/Kernel/Include/Runtime.hpp
 * @brief Kernel C++ runtime hooks.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

namespace Quantum::System::Kernel {
  /**
   * Runs global constructors in the init array.
   */
  void RunGlobalConstructors();
}
