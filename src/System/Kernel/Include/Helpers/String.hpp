//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Helpers/String.hpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Simple string helper utilities.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Helpers {
  class String {
    public:
      /**
       * Converts a signed 32-bit integer to a decimal C-string into a caller
       * buffer.
       * @param value Integer to convert.
       * @param buffer Destination buffer.
       * @param length Size of the destination buffer.
       * @return True on success; false if the buffer is too small.
       */
      static bool ToString(int32 value, char* buffer, usize length);

      /**
       * Returns the length of a null-terminated string (excluding the null).
       */
      static usize Length(const char* str);

      /**
       * Concatenates two C-strings into a destination buffer.
       * @param left First string.
       * @param right Second string.
       * @param buffer Destination buffer.
       * @param length Size of the destination buffer.
       * @return True on success; false if the buffer is too small.
       */
      static bool Concat(
        const char* left,
        const char* right,
        char* buffer,
        usize length
      );

      /**
       * Concatenates two C-strings into a destination buffer, computing the
       * required length automatically.
       * @param left First string.
       * @param right Second string.
       * @param buffer Destination buffer.
       * @return true on success; false if the buffer is too small.
       */
      static bool Concat(
        const char* left,
        const char* right,
        char* buffer
      );
  };
}
