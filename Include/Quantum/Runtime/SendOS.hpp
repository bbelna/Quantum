/**
 * @file Include/Quantum/Runtime/SendOS.hpp
 * @brief Declares @ref SendOS.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

/**
 * @brief Sends a fire-and-forget message to the specified Operating System
 *        (OS) subsystem. No reply is expected.
 * @param subsystem Subsystem identifier (IPC port ID).
 * @param payload The request payload to send.
 */
template <typename PayloadType>
void SendOS(UInt32 subsystem, PayloadType payload);
