/**
 * @file Servers/Stream/ManagedStream.hpp
 * @brief Declares @ref @QStrmSrv::ManagedStream.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <StreamServerTypes.hpp>

namespace Quantum::Servers::Stream {
  /**
   * @brief Tracks the lifecycle of a single managed stream.
   */
  struct ManagedStream {
    /**
     * @brief The @ref SharedBufferID backing this @ref ManagedStream.
     * @note A value of `0` indicates an unused slot.
     */
    SharedBufferID BufferID = 0;

    /**
     * @brief The server's mapping of the shared buffer.
     *
     * Kept alive so the buffer is not freed before clients attach.
     */
    UIntPtr BufferAddress = 0;

    /**
     * @brief Whether the writer side has been closed.
     */
    bool WriterClosed = false;

    /**
     * @brief Whether the reader side has been closed.
     */
    bool ReaderClosed = false;
  };
}
