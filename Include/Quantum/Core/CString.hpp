/**
 * @file Include/Quantum/Core/CString.hpp
 * @brief Declares @ref @QCore::CString.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Types.hpp"

/**
 * @brief C-string helper utilities.
 */
namespace Quantum::Core::CString {
  /**
   * @brief Standard format buffer size for C-string operations.
   */
  constexpr Size FormatBufferSize = 512;

  /**
   * @brief
   *   Converts a signed 32-bit integer to a decimal C-string into a caller
   *   buffer.
   * @param value Integer to convert.
   * @param buffer Destination buffer.
   * @param length Size of the destination buffer.
   * @return `true` on success; `false` if the buffer is too small.
   */
  bool ToCString(Int32 value, char* buffer, Size length);

  /**
   * @brief
   *   Converts a signed 32-bit integer to a decimal C-string using an
   *   internal static buffer (overwritten on each call).
   * @param value Integer to convert.
   * @return Pointer to the static null-terminated string.
   */
  char* ToCString(Int32 value);

  /**
   * @brief
   *   Returns the length of a null-terminated string excluding the null
   *   terminator.
   * @param str String to measure.
   * @return String length.
   */
  Size Length(const char* str);

  /**
   * @brief Returns the length of a null-terminated string (bounded).
   * @param str String to measure.
   * @param maxLength Maximum length to measure.
   * @return String length up to `maxLength`.
   */
  Size Length(const char* str, Size maxLength);

  /**
   * @brief Concatenates two C-strings into a destination buffer.
   * @param left First string.
   * @param right Second string.
   * @param buffer Destination buffer.
   * @param length Size of the destination buffer.
   * @return `true` on success; `false` if the buffer is too small.
   */
  bool Concat(
    const char* left,
    const char* right,
    char* buffer,
    Size length
  );

  /**
   * @brief
   *   Concatenates two C-strings into a destination buffer, computing the
   *   required length automatically.
   * @param left First string.
   * @param right Second string.
   * @param buffer Destination buffer.
   * @return `true` on success; `false` if the buffer is too small.
   */
  bool Concat(const char* left, const char* right, char* buffer);

  /**
   * @brief
   *   Formats a C-string into a destination buffer using a simple format
   *   string and a variable argument list.
   * @param buffer Destination buffer.
   * @param length Size of the destination buffer.
   * @param format Format string.
   * @param args Variable argument list.
   * @return `true` on success; `false` if the buffer is too small.
   */
  bool FormatV(
    char* buffer,
    Size length,
    const char* format,
    VariableArgumentsList args
  );

  /**
   * @brief
   *   Formats a C-string into a destination buffer using a simple format
   *   string and a variable argument list.
   * @param buffer Destination buffer.
   * @param length Size of the destination buffer.
   * @param format Format string.
   * @param ... Variable argument list.
   * @return `true` on success; `false` if the buffer is too small.
   */
  bool Format(
    char* buffer,
    Size length,
    const char* format,
    ...
  );

  /**
   * @brief
   *   Concatenates two C-strings and formats the result into a destination
   *   buffer using a variable argument list.
   * @param left First string.
   * @param right Second string.
   * @param buffer Destination buffer.
   * @param length Size of the destination buffer.
   * @param args Variable argument list.
   * @return `true` on success; `false` if the buffer is too small.
   */
  bool ConcatAndFormatV(
    const char* left,
    const char* right,
    char* buffer,
    Size length,
    VariableArgumentsList args
  );

  /**
   * @brief
   *   Concatenates two C-strings and formats the result into a destination
   *   buffer.
   * @param left First string.
   * @param right Second string.
   * @param buffer Destination buffer.
   * @param length Size of the destination buffer.
   * @param ... Additional arguments for formatting.
   * @return `true` on success; `false` if the buffer is too small.
   */
  bool ConcatAndFormat(
    const char* left,
    const char* right,
    char* buffer,
    Size length,
    ...
  );

  /**
   * @brief
   *   Copies a C-string into a destination buffer with a maximum byte
   *   count.
   * @param source C-string source.
   * @param destination Destination buffer.
   * @param maxBytes
   *   Maximum number of bytes to copy (including null terminator).
   * @return Number of bytes copied (including null terminator).
   */
  UInt32 Copy(
    const char* source,
    UInt8* destination,
    UInt32 maxBytes
  );

  /**
   * @brief
   *   Copies a C-string into a destination buffer with a maximum byte
   *   count.
   * @param source C-string source.
   * @param destination Destination buffer.
   * @param maxBytes
   *   Maximum number of bytes to copy (including null terminator).
   * @return Number of bytes copied (including null terminator).
   */
  UInt32 Copy(
    const char* source,
    char* destination,
    UInt32 maxBytes
  );

  /**
   * @brief Lexicographically compares two null-terminated C-strings.
   * @param left First string.
   * @param right Second string.
   * @return Negative if `left` < `right`, zero if equal, positive if
   *         `left` > `right`.
   */
  Int32 Compare(const char* left, const char* right);

  /**
   * @brief Compares two C-strings for equality up to a specified length.
   * @param left First string.
   * @param right Second string.
   * @param length Number of characters to compare.
   * @return `true` if they are equal; `false` otherwise.
   */
  bool Equals(const char* left, const char* right, Size length);

  /**
   * @brief Compares two C-strings for equality.
   * @param left First string.
   * @param right Second string.
   * @return `true` if they are equal; `false` otherwise.
   */
  bool Equals(const char* left, const char* right);

  /**
   * @brief Sets a block of memory to a given byte value.
   * @param destination Destination buffer.
   * @param value Byte value to set.
   * @param count Number of bytes to write.
   */
  void Set(void* destination, UInt8 value, Size count);

  /**
   * @brief Determines if a C-string starts with a given prefix.
   * @param str String to test.
   * @param prefix Prefix to match.
   * @return `true` if the prefix matches; `false` otherwise.
   */
  bool StartsWith(const char* str, const char* prefix);

  /**
   * @brief Writes @p str left-aligned into @p buffer, padding the
   *        remaining space with @p fill up to @p fieldWidth. Always
   *        null-terminates.
   * @param str The string to write.
   * @param buffer Destination buffer (must be at least
   *        `fieldWidth + 1` bytes).
   * @param bufferSize Size of @p buffer in bytes.
   * @param fieldWidth The total field width in characters.
   * @param fill Padding character (default: space).
   * @return Number of characters written (excluding null terminator).
   */
  Size PadRight(
    const char* str,
    char* buffer,
    Size bufferSize,
    Size fieldWidth,
    char fill = ' '
  );

  /**
   * @brief Writes @p str right-aligned into @p buffer, padding the
   *        leading space with @p fill up to @p fieldWidth. Always
   *        null-terminates.
   * @param str The string to write.
   * @param buffer Destination buffer (must be at least
   *        `fieldWidth + 1` bytes).
   * @param bufferSize Size of @p buffer in bytes.
   * @param fieldWidth The total field width in characters.
   * @param fill Padding character (default: space).
   * @return Number of characters written (excluding null terminator).
   */
  Size PadLeft(
    const char* str,
    char* buffer,
    Size bufferSize,
    Size fieldWidth,
    char fill = ' '
  );
}
