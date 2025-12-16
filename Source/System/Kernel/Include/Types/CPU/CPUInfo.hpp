/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Types/CPU/CPUInfo.hpp
 * Arch-independent CPU information.
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Types::CPU {
  /**
   * Arch-independent CPU information.
   */
  struct CPUInfo {
    /**
     * The CPU's vendor name.
     */
    char vendor[32];

    /**
     * The CPU model/description.
     */
    char model[64];

    /**
     * The number of CPU cores.
     */
    UInt32 coreCount;

    /**
     * Whether the CPU supports SIMD instructions.
     */
    bool hasSIMD;

    /**
     * Whether the CPU has a hardware FPU.
     */
    bool hasHardwareFPU;

    /**
     * Whether the CPU supports virtualization extensions.
     */
    bool hasVirtualization;
  };
}