/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Helpers/CStringHelper.hpp
 * C-string helper utilities.
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Helpers {
  /**
   * C-string helper utilities.
   */
  class CStringHelper {
    public:
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
      static bool ToCString(Int32 value, CStringMutable buffer, Size length);

      /**
       * Converts a signed 32-bit integer to a decimal C-string using an
       * internal static buffer (overwritten on each call).
       * @param value
       *   Integer to convert.
       * @return
       *   Pointer to the static null-terminated string.
       */
      static char* ToCString(Int32 value);

      /**
       * Returns the length of a null-terminated string (excluding the null).
       */
      static Size Length(CString str);

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
      static bool Concat(
        CString left,
        CString right,
        CStringMutable buffer,
        Size length
      );

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
      static bool Concat(
        CString left,
        CString right,
        CStringMutable buffer
      );

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
      static bool Format(
        CStringMutable buffer,
        Size length,
        CString format,
        VariableArgumentsList args
      );

    private:
      /**
       * Buffer size for integer to C-string conversions.
       */
      static constexpr Size _bufferSize = 12;

      /**
       * Static buffer for integer to C-string conversions.
       */
      static char _staticBuffer[_bufferSize];

      /**
       * Writes an integer value to the given buffer as a C-string.
       * @param value
       *   The integer value.
       * @param buffer
       *   The output buffer.
       * @param length
       *   The length of the output buffer.
       * @return
       *   True on success, false if the buffer is too small.
       */
      static bool WriteIntToBuffer(
        Int32 value,
        CStringMutable buffer,
        Size length
      );

      /**
       * Appends a single character to `buffer[out]`, respecting length.
       * @param buffer
       *   Destination buffer.
       * @param length
       *   Size of the destination buffer.
       * @param out
       *   Current output index (updated on success).
       * @param c
       *   Character to append.
       * @return
       *   True on success; false if buffer is too small.
       */
      static bool AppendChar(
        CStringMutable buffer,
        Size length,
        Size& out,
        char c
      );

      /**
       * Appends a null-terminated string to `buffer[out]`, respecting length.
       * @param buffer
       *   Destination buffer.
       * @param length
       *   Size of the destination buffer.
       * @param out
       *   Current output index (updated on success).
       * @param str
       *   String to append.
       * @return
       *   True on success; false if buffer is too small.
       */
      static bool AppendString(
        CStringMutable buffer,
        Size length,
        Size& out,
        CString str
      );

      /**
       * Appends an unsigned integer in the given base to `buffer[out]`,
       * respecting length.
       * @param buffer
       *   Destination buffer.
       * @param length
       *   Size of the destination buffer.
       * @param out
       *   Current output index (updated on success).
       * @param value
       *   Unsigned integer to append.
       * @param base
       *   Base to use (2-16).
       * @param prefixHex
       *   If true and base is 16, prefixes with "0x".
       * @return
       *   True on success; false if buffer is too small.
       */
      static bool AppendUnsigned(
        CStringMutable buffer,
        Size length,
        Size& out,
        UInt32 value,
        UInt32 base,
        bool prefixHex = false
      );
  };
}
