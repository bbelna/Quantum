/**
 * @file Libraries/Quantum/Include/CString.hpp
 * @brief C-string helper utilities.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Types.hpp"

namespace Quantum {
  /**
   * Converts a signed 32-bit integer to a decimal C-string into a caller
   * buffer.
   * @param value
   *   Integer to convert.
   * @param buffer
   *   Destination buffer.
   * @param length
   *   Size of the destination buffer.
   * @return
   *   True on success; false if the buffer is too small.
   */
  bool ToCString(Int32 value, CStringMutable buffer, Size length);

  /**
   * Converts a signed 32-bit integer to a decimal C-string using an
   * internal static buffer (overwritten on each call).
   * @param value
   *   Integer to convert.
   * @return
   *   Pointer to the static null-terminated string.
   */
  char* ToCString(Int32 value);

  /**
   * Returns the length of a null-terminated string (excluding the null).
   */
  Size Length(CString str);

  /**
   * Concatenates two C-strings into a destination buffer.
   * @param left
   *   First string.
   * @param right
   *   Second string.
   * @param buffer
   *   Destination buffer.
   * @param length
   *   Size of the destination buffer.
   * @return
   *   True on success; false if the buffer is too small.
   */
  bool Concat(CString left, CString right, CStringMutable buffer, Size length);

  /**
   * Concatenates two C-strings into a destination buffer, computing the
   * required length automatically.
   * @param left
   *   First string.
   * @param right
   *   Second string.
   * @param buffer
   *   Destination buffer.
   * @return
   *   True on success; false if the buffer is too small.
   */
  bool Concat(CString left, CString right, CStringMutable buffer);

  /**
   * Formats a C-string into a destination buffer using a simple format
   * string and a variable argument list.
   * @param buffer
   *   Destination buffer.
   * @param length
   *   Size of the destination buffer.
   * @param format
   *   Format string.
   * @param args
   *   Variable argument list.
   * @return
   *   True on success; false if the buffer is too small.
   */
  bool Format(
    CStringMutable buffer,
    Size length,
    CString format,
    VariableArgumentsList args
  );
}
