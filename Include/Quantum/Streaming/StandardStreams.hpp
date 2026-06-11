/**
 * @file Include/Quantum/Streaming/StandardStreams.hpp
 * @brief Declares the standard stream globals (stdin, stdout, stderr).
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Stream.hpp"

namespace Quantum::Streaming {
  /**
   * @brief Standard input stream, pre-initialized by CRT0 from the process
   *        stream table if the parent provided inherited streams. Check
   *        @ref Stream::IsValid before use.
   */
  extern Stream& StandardIn;

  /**
   * @brief Standard output stream, pre-initialized by CRT0 from the process
   *        stream table if the parent provided inherited streams. Check
   *        @ref Stream::IsValid before use.
   */
  extern Stream& StandardOut;

  /**
   * @brief Standard error stream, pre-initialized by CRT0 from the process
   *        stream table if the parent provided inherited streams. Check
   *        @ref Stream::IsValid before use.
   */
  extern Stream& StandardError;

  /**
   * @brief Initializes the standard streams from the process stream table.
   *        Called by CRT0 before `Main()`. Applications should not call
   *        this directly.
   */
  void InitializeStandardStreams();

  /**
   * @brief Closes all open standard streams. Called by CRT0 after `Main()`
   *        returns. Applications should not call this directly.
   */
  void CloseStandardStreams();
}
