/**
 * @file Include/Quantum/Servers/Stream/StreamConstants.hpp
 * @brief Declares constants for the stream server.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "StreamTypes.hpp"

namespace Quantum::Servers::Stream {
  /**
   * @brief ABI version for the stream server protocol.
   */
  constexpr UInt32 StreamABIVersion = 1;

  /**
   * @brief IPC port ID for the stream server.
   */
  constexpr IPCPortID StreamPortID = 11;

  /**
   * @brief Maximum number of concurrently managed streams.
   */
  constexpr Size MaxStreams = 64;
}
