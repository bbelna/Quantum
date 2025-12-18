/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Types/Tasks/TSS.hpp
 * IA32 Task State Segment structure.
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32::Types::Tasks {
  /**
   * IA32 TSS structure.
   */
  struct [[gnu::packed]] TSS32 {
    UInt32 PreviousTSS;
    UInt32 ESP0;
    UInt32 SS0;
    UInt32 ESP1;
    UInt32 SS1;
    UInt32 ESP2;
    UInt32 SS2;
    UInt32 CR3;
    UInt32 EIP;
    UInt32 EFlags;
    UInt32 EAX;
    UInt32 ECX;
    UInt32 EDX;
    UInt32 EBX;
    UInt32 ESP;
    UInt32 EBP;
    UInt32 ESI;
    UInt32 EDI;
    UInt32 ES;
    UInt32 CS;
    UInt32 SS;
    UInt32 DS;
    UInt32 FS;
    UInt32 GS;
    UInt32 LDT;
    UInt16 Trap;
    UInt16 IOMapBase;
  };
}
