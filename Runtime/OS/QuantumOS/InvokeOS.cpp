/**
 * @file Runtime/QuantumInvokeOS.cpp
 * @brief Implements @ref InvokeOS and @ref InvokeOSEx for QuantumOS.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "QuantumOSRuntimeTypes.hpp"

/**
 * @brief QuantumOS specialization of @ref InvokeOS.
 * @param subsystem Subsystem code (`0` for direct kernel call).
 * @param operation Operation code to pass in `EAX`.
 * @param payload @ref KernelOperationPayload with arguments to pass in
 *                `EBX`, `ESI`, and `EDI`.
 *
 * Handles subsystem `0` (direct kernel system call) by issuing a `SYSENTER`
 * with the @ref KernelOperationPayload arguments mapped to registers.
 * Non-zero subsystems are not valid for this specialization and return `-1`;
 * use the generic @ref InvokeOS template with the appropriate server
 * @ref ABIRequestWithReplyPort payload instead.
 */
template <>
UInt32 InvokeOS<UInt32, KernelOperationPayload>(
  UInt32 subsystem,
  UInt32 operation,
  KernelOperationPayload payload
) {
  if (subsystem != 0) {
    return static_cast<UInt32>(-1);
  }

  #if defined (ARCH_IA32)
  UInt32 result;
  UInt32 arg1 = payload.Arg1;
  UInt32 arg2 = payload.Arg2;
  UInt32 arg3 = payload.Arg3;

  asm volatile(
    "movl %%esp, %%ecx\n"
    "leal 1f, %%edx\n"
    "sysenter\n"
    "1:"
    : "=a"(result),
      "+b"(arg1),
      "+S"(arg2),
      "+D"(arg3)
    : "0"(operation)
    : "memory",
      "cc",
      "ecx",
      "edx"
  );

  return result;
  #else
  #error "InvokeOS not implemented for this arch"
  #endif
}

/**
 * @brief QuantumOS specialization of @ref InvokeOSEx.
 * @param operation Operation code to pass in `EAX`.
 * @param payload @ref KernelOperationPayload with arguments to pass in
 *                `EBX`, `ESI`, and `EDI`.
 * @return @ref KernelOperationResult containing the result.
 */
KernelOperationResult InvokeOSEx(
  UInt32 operation,
  KernelOperationPayload payload
) {
  #if defined (ARCH_IA32)
  UInt32 result;
  UInt32 arg1 = payload.Arg1;
  UInt32 arg2 = payload.Arg2;
  UInt32 arg3 = payload.Arg3;

  asm volatile(
    "movl %%esp, %%ecx\n"
    "leal 1f, %%edx\n"
    "sysenter\n"
    "1:"
    : "=a"(result),
      "+b"(arg1),
      "+S"(arg2),
      "+D"(arg3)
    : "0"(operation)
    : "memory",
      "cc",
      "ecx",
      "edx"
  );

  return { result, arg1, arg2, arg3 };
  #else
  #error "InvokeOSEx not implemented for this arch"
  #endif
}
