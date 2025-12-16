/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Helpers/CStringHelper.hpp
 * C-string helper utilities.
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::Kernel::Helpers {
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
  };
}
