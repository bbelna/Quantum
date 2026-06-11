/**
 * @file Include/Quantum/ABI/ABITypes.hpp
 * @brief Declares types for @ref @QABI.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Kernel/Types.hpp>

namespace Quantum::ABI {
  /**
   * @brief ID for an IPC port resource.
   */
  using IPCPortResourceID = Kernel::IPC::IPCPortResourceID;

  /**
   * @brief ID for an IPC port.
   */
  using IPCPortID = Kernel::IPC::IPCPortID;

  /**
   * @brief Rights for an IPC port.
   */
  using IPCPortRights = Kernel::IPC::IPCPortRights;

  /**
   * @brief Represents an IPC message.
   */
  using IPCMessage = Kernel::IPC::IPCMessage;
}
