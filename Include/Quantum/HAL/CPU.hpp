/**
 * @file Include/Quantum/HAL/CPU.hpp
 * @brief Declares CPU driver operations and payload types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::HAL::CPU {
  /**
   * @brief Operations supported by the CPU platform driver.
   */
  enum class CPUDriverOperation : UInt32 {
    /**
     * @brief Queries whether the system is running under a hypervisor.
     *
     * Payload: pointer to a @ref CPUHypervisorInfo struct to fill.
     * Returns 1 if a hypervisor is detected, 0 otherwise.
     */
    GetHypervisorInfo = 1,

    /**
     * @brief Queries FPU presence and integration status.
     *
     * Payload: pointer to a @ref CPUFPUInfo struct to fill.
     * Returns 1 if an FPU is present, 0 otherwise.
     */
    GetFPUInfo = 2
  };

  /**
   * @brief Hypervisor identification returned by @ref
   *        CPUDriverOperation::GetHypervisorInfo.
   */
  struct CPUHypervisorInfo {
    /**
     * @brief `true` if a hypervisor is present (CPUID leaf 1, ECX bit 31).
     */
    bool Present;

    /**
     * @brief Null-terminated 12-character vendor string from CPUID leaf
     *        `0x40000000` (EBX + ECX + EDX). Examples: `"KVMKVMKVM\0\0\0"`,
     *        `"TCGTCGTCGTCG"`, `"VMwareVMware"`.
     */
    char Vendor[13];
  };

  /**
   * @brief FPU detection result returned by @ref
   *        CPUDriverOperation::GetFPUInfo.
   */
  struct CPUFPUInfo {
    /**
     * @brief `true` if an x87 FPU responded to the `FNINIT`/`FNSTSW` probe.
     */
    bool Present;

    /**
     * @brief `true` if the FPU is integrated on-die (family >= 5, or CPUID
     *        feature bit EDX bit 0 set).
     */
    bool Integrated;
  };
}
