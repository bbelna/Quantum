/**
 * @file Kernel/Concurrency/ThreadPriority.hpp
 * @brief Declares @ref @QKrnl::Concurrency::ThreadPriority.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Enum.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Priority levels for thread scheduling.
   *
   * 32 levels (`0`-`31`) split into two bands:
   *   - Time-Sharing (`0`-`15`): user applications, with priority decay
   *   - Real-Time (`16`-`31`): system servers, strict priority ordering
   */
  enum class ThreadPriority : UInt8 {
    /**
     * @brief Idle priority; runs only when no other thread is ready.
     */
    Idle = 0,

    /**
     * @brief Lowest user-application priority.
     */
    UserLowest = 1,

    /**
     * @brief Low user-application priority.
     */
    UserLow = 5,

    /**
     * @brief Default priority for normal user-application threads.
     */
    UserNormal = 10,

    /**
     * @brief Highest user-application priority (still time-sharing band).
     */
    UserHigh = 15,

    /**
     * @brief Upper bound of the time-sharing band (same value as UserHigh).
     */
    TimeSharingMax = 15,

    /**
     * @brief Lower bound of the real-time band.
     */
    RealTimeMin = 16,

    /**
     * @brief Base priority for system server threads.
     */
    ServerBase = 16,

    /**
     * @brief Priority for device-driver server threads.
     */
    DeviceServer = 17,

    /**
     * @brief Priority for file-system server threads.
     */
    FileSystemServer = 18,

    /**
     * @brief Priority for storage server threads.
     */
    StorageServer = 19,

    /**
     * @brief Priority for graphics server threads.
     */
    GraphicsServer = 20,

    /**
     * @brief Priority for input device driver threads.
     */
    InputDriver = 22,

    /**
     * @brief Priority for input server threads.
     */
    InputServer = 24,

    /**
     * @brief Priority for display-server render threads.
     */
    DisplayServerRender = 25,

    /**
     * @brief Priority for display-server management threads.
     */
    DisplayServer = 27,

    /**
     * @brief Highest priority; reserved for system-critical threads.
     */
    SystemCritical = 31,

    /**
     * @brief Maximum valid priority value (alias for @ref SystemCritical).
     */
    Maximum = 31,
  };

  /**
   * @brief Total number of priority levels (array dimension, not a valid
   *        priority).
   */
  constexpr Size ThreadPriorityCount = 32;
}

QUANTUM_ENABLE_ENUM_VALUE_OPS(Quantum::Kernel::Concurrency, ThreadPriority)
