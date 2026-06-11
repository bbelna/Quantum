/**
 * @file Include/Quantum/Servers/Stream/StreamOperation.hpp
 * @brief Declares @ref StreamOperation.
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
   * @brief Operations supported by the stream server.
   */
  enum class StreamOperation : UInt32 {
    /**
     * @brief Creates a new stream backed by a shared-memory ring buffer.
     *        Returns the @ref SharedBufferID for the caller
     *        to attach.
     */
    CreateStream = 1,

    /**
     * @brief Notifies the server that the writer side has closed.
     *        Fire-and-forget; no reply is expected.
     */
    CloseWriter = 2,

    /**
     * @brief Notifies the server that the reader side has closed.
     *        Fire-and-forget; no reply is expected.
     */
    CloseReader = 3
  };
}
