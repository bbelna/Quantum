/**
 * @file Include/Quantum/Core/Byte.hpp
 * @brief Declares @ref @QCore::Byte.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Types.hpp"

/**
 * @brief Byte manipulation helper utilities.
 */
namespace Quantum::Core::Byte {
  /**
   * @brief Copies a sequence of bytes from source to destination.
   * @param destination Destination buffer.
   * @param source Source buffer.
   * @param length Number of bytes to copy.
   */
  void Copy(void* destination, const void* source, Size length);

  /**
   * @brief Copies a sequence of bytes from source to destination.
   * @param destination Destination buffer.
   * @param source Source buffer.
   * @param length Number of bytes to copy.
   */
  void Copy(UInt8* destination, UInt8* source, Size length);

  /**
   * @brief Fills a sequence of bytes in the given destination with the
   *        specified value.
   * @param destination Destination buffer.
   * @param value Value to fill.
   * @param length Number of bytes to fill.
   */
  void Fill(void* destination, UInt8 value, Size length);

  /**
   * @brief Zeros a sequence of bytes in the given destination.
   * @param destination Destination buffer.
   * @param length Number of bytes to zero.
   */
  void Zero(void* destination, Size length);
}
