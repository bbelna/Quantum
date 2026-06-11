/**
 * @file Include/Quantum/Servers/Run/ABI/LoadELFRequest.hpp
 * @brief Declares @ref LoadELFRequest.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Memory.hpp>

#include "../RunServerConstants.hpp"
#include "../RunServerRequest.hpp"

namespace Quantum::Servers::Run::ABI {
  /**
   * @brief Request to load an ELF binary. The raw ELF data follows
   *        immediately after this struct in the IPC payload, followed
   *        by packed argument strings (if any).
   */
  struct LoadELFRequest : public RunServerRequestWithReply {
    /**
     * @brief Human-readable name for the new process.
     */
    char Name[64];

    /**
     * @brief Initial working directory for the new process. The run server
     *        copies this verbatim into the process entry's working
     *        directory.
     */
    char WorkingDirectory[MaxWorkingDirectoryLength];

    /**
     * @brief Size of the ELF binary data in bytes. The ELF data starts
     *        immediately after this struct; argument data (if any) follows
     *        at offset `sizeof(LoadELFRequest) + ELFSize`.
     */
    Size ELFSize;

    /**
     * @brief Number of command-line arguments packed after the ELF data.
     */
    Size ArgumentCount;

    /**
     * @brief Number of inherited stream buffer IDs (0 to 3).
     */
    UInt8 StreamCount = 0;

    /**
     * @brief Inherited stream buffer IDs. Index 0 = stdin, 1 = stdout,
     *        2 = stderr. A value of `0` means no stream for that index.
     */
    SharedBufferID StreamBufferIDs[3] = {};
  };
}
