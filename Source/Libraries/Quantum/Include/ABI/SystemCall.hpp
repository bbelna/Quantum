/**
 * @file Libraries/Quantum/Include/ABI/SystemCall.hpp
 * @brief System call invocation interface.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Types.hpp"

namespace Quantum::ABI {
  /**
   * System call identifiers.
   */
  enum class SystemCall : UInt32 {
    Task_Exit = 100,
    Task_Yield = 101,
    Task_GrantIOAccess = 102,
    Console_Write = 200,
    Console_WriteLine = 201,
    InitBundle_GetInfo = 300,
    InitBundle_SpawnTask = 301,
    IPC_CreatePort = 400,
    IPC_Send = 401,
    IPC_Receive = 402,
    IPC_TryReceive = 403,
    IRQ_Register = 501,
    IRQ_Unregister = 502,
    IRQ_Enable = 503,
    IRQ_Disable = 504,
    IO_In8 = 600,
    IO_In16 = 601,
    IO_In32 = 602,
    IO_Out8 = 603,
    IO_Out16 = 604,
    IO_Out32 = 605,
    Block_GetCount = 700,
    Block_GetInfo = 701,
    Block_Read = 702,
    Block_Write = 703,
    Block_Bind = 704,
    Block_AllocateDMABuffer = 705,
    Block_UpdateInfo = 706,
    Block_Register = 707,
    Memory_ExpandHeap = 800
  };

  /**
   * Invokes a system call via `int 0x80`.
   * @param call
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
  inline UInt32 InvokeSystemCall(
    SystemCall call,
    UInt32 arg1 = 0,
    UInt32 arg2 = 0,
    UInt32 arg3 = 0
  ) {
    UInt32 result = 0;

    asm volatile(
      "int $0x80"
      : "=a"(result)
      : "a"(static_cast<UInt32>(call)),
        "b"(arg1),
        "c"(arg2),
        "d"(arg3)
      : "memory"
    );

    return result;
  }
}
