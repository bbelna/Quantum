/**
 * @file Libraries/Quantum/Include/Bytes.hpp
 * @brief Byte manipulation helper utilities.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "Types.hpp"

namespace Quantum {
  /**
   * Copies a sequence of bytes from source to destination.
   * @param destination
   *   Destination buffer.
   * @param source
   *   Source buffer.
   * @param length
   *   Number of bytes to copy.
   */
  void CopyBytes(void* destination, const void* source, UInt32 length);
}
