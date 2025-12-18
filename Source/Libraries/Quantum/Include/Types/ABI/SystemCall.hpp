/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/Types/ABI/SystemCall.hpp
 * User-mode system call invocation helpers.
 */

#pragma once

#include <Types/ABI/SystemCallId.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::Types::ABI {
  /**
   * Invokes a system call via int 0x80.
   * @param callId
   *   System call identifier.
   * @param arg1
   *   First argument (EBX).
   * @param arg2
   *   Second argument (ECX).
   * @param arg3
   *   Third argument (EDX).
   * @return
   *   Result returned in EAX.
   */
  inline UInt32 Invoke(
    SystemCallId callId,
    UInt32 arg1 = 0,
    UInt32 arg2 = 0,
    UInt32 arg3 = 0
  ) {
    UInt32 result = 0;

    asm volatile(
      "int $0x80"
      : "=a"(result)
      : "a"(static_cast<UInt32>(callId)),
        "b"(arg1),
        "c"(arg2),
        "d"(arg3)
      : "memory"
    );

    return result;
  }
}
