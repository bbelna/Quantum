/**
 * @file System/Kernel/UserMode.cpp
 * @brief User mode transition handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include "Arch/UserMode.hpp"
#include "UserMode.hpp"

namespace Quantum::System::Kernel {
  bool UserMode::MapUserStack(UInt32 userStackTop, UInt32 sizeBytes) {
    return Arch::UserMode::MapUserStack(userStackTop, sizeBytes);
  }

  void UserMode::Enter(UInt32 entryPoint, UInt32 userStackTop) {
    Arch::UserMode::Enter(entryPoint, userStackTop);
  }
}
