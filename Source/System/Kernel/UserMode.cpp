/**
 * @file System/Kernel/UserMode.cpp
 * @brief User mode transition handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include "UserMode.hpp"

#if defined(QUANTUM_ARCH_IA32)
#include "Arch/IA32/Prelude.hpp"
#include "Arch/IA32/UserMode.hpp"
#endif

namespace Quantum::System::Kernel {
  bool UserMode::MapUserStack(UInt32 userStackTop, UInt32 sizeBytes) {
    #if defined(QUANTUM_ARCH_IA32)
    return KernelIA32::UserMode::MapUserStack(userStackTop, sizeBytes);
    #else
    (void)userStackTop;
    (void)sizeBytes;

    return false;
    #endif
  }

  void UserMode::Enter(UInt32 entryPoint, UInt32 userStackTop) {
    #if defined(QUANTUM_ARCH_IA32)
    KernelIA32::UserMode::Enter(entryPoint, userStackTop);
    #else
    (void)entryPoint;
    (void)userStackTop;

    for (;;) {}
    #endif
  }
}
