/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/ABI/SystemCall.hpp
 * System call invocation interface.
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
    IO_In8 = 500,
    IO_In16 = 501,
    IO_In32 = 502,
    IO_Out8 = 503,
    IO_Out16 = 504,
    IO_Out32 = 505,
    Block_GetCount = 600,
    Block_GetInfo = 601,
    Block_Read = 602,
    Block_Write = 603,
    Block_Bind = 604,
    Block_AllocateDMABuffer = 605,
    Block_UpdateInfo = 606,
    FileSystem_ListVolumes = 700,
    FileSystem_GetVolumeInfo = 701,
    FileSystem_SetVolumeLabel = 702,
    FileSystem_OpenVolume = 703,
    FileSystem_CloseVolume = 704,
    FileSystem_Open = 705,
    FileSystem_Close = 706,
    FileSystem_Read = 707,
    FileSystem_Write = 708,
    FileSystem_Seek = 709,
    FileSystem_Stat = 710,
    FileSystem_ReadDirectory = 711,
    FileSystem_CreateDirectory = 712,
    FileSystem_CreateFile = 713,
    FileSystem_Remove = 714,
    FileSystem_Rename = 715,
    FileSystem_RegisterService = 716
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
