/**
 * @file Include/Quantum/Kernel/Arch/IA32/ABI.hpp
 * @brief Declaration of the IA-32 architecture ABI namespace.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>
#include <Quantum/Kernel/KernelOperation.hpp>

namespace Quantum::Kernel::ABI {
  using Quantum::Kernel::KernelOperation;

  /**
   * @brief Invokes the kernel's ABI via SYSENTER.
   * @param operation The ABI operation to perform.
   * @param arg1 First argument (EBX).
   * @param arg2 Second argument (ESI -> remapped to ECX by kernel entry).
   * @param arg3 Third argument (EDI -> remapped to EDX by kernel entry).
   * @return The result of the ABI operation (EAX).
   */
  inline UInt32 Invoke(
    KernelOperation operation,
    UInt32 arg1,
    UInt32 arg2,
    UInt32 arg3
  ) {
    UInt32 result;

    asm volatile(
      "movl %%esp, %%ecx\n"
      "leal 1f, %%edx\n"
      "sysenter\n"
      "1:"
      : "=a"(result),
        "+b"(arg1),
        "+S"(arg2),
        "+D"(arg3)
      : "0"(static_cast<UInt32>(operation))
      : "memory",
        "cc",
        "ecx",
        "edx"
    );

    return result;
  }
}
