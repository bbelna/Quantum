/**
 * @file System/Kernel/Include/Objects/KernelObjectType.hpp
 * @brief Kernel object type identifiers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Objects {
  /**
   * Kernel object type identifiers.
   */
  enum class KernelObjectType : UInt32 {
    /**
     * No object type.
     */
    None = 0,

    /**
     * IPC port object.
     */
    IPCPort = 1,

    /**
     * Block device object.
     */
    BlockDevice = 2,

    /**
     * Input device object.
     */
    InputDevice = 3,

    /**
     * IRQ line object.
     */
    IRQLine = 4
  };
}
