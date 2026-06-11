/**
 * @file Include/Quantum/Streaming/ProcessStreamTable.hpp
 * @brief Declares the process stream table layout for inherited streams.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Kernel.hpp>
#include <Quantum/Memory.hpp>

namespace Quantum::Streaming {
  /**
   * @brief Fixed virtual address where the kernel maps the process stream
   *        table in each child process that inherits streams.
   */
  inline constexpr UIntPtr ProcessStreamTableAddress = 0x0FFFF000;

  /**
   * @brief Maximum number of inherited stream entries.
   */
  inline constexpr Size MaxProcessStreams = 3;

  /**
   * @brief Stream index constants.
   */
  namespace StreamIndex {
    /**
     * @brief Standard input stream index.
     */
    inline constexpr UInt8 StandardIn = 0;

    /**
     * @brief Standard output stream index.
     */
    inline constexpr UInt8 StandardOut = 1;

    /**
     * @brief Standard error stream index.
     */
    inline constexpr UInt8 StandardError = 2;
  }

  /**
   * @brief Layout of the process stream table mapped at
   *        @ref ProcessStreamTableAddress.
   *
   * The kernel populates this structure during process spawn when the parent
   * provides stream buffer IDs. The CRT0 runtime reads it before calling
   * `Main()` to initialize the standard stream globals.
   */
  struct ProcessStreamTable {
    /**
     * @brief Number of valid entries (0 to @ref MaxProcessStreams).
     */
    UInt8 Count;

    /**
     * @brief Reserved, must be `0`.
     */
    UInt8 Reserved[3];

    /**
     * @brief Shared buffer IDs for each stream. Index 0 = stdin,
     *        1 = stdout, 2 = stderr. A value of `0` means no stream.
     */
    SharedBufferID Entries[MaxProcessStreams];
  } __attribute__((packed));
}
