/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/ABI/Console.hpp
 * User-mode console wrapper.
 */

#pragma once

#include "ABI/SystemCall.hpp"
#include "Types.hpp"

namespace Quantum::ABI {
  /**
   * User-mode console wrapper.
   */
  class Console {
    public:
      /**
       * Writes a string to the console.
       * @param str
       *   Null-terminated string to write.
       */
      static inline void Write(CString str) {
        if (!str) {
          return;
        }

        UInt32 length = StringLength(str);

        if (length == 0) {
          return;
        }

        ABI::InvokeSystemCall(
          ABI::SystemCall::Console_Write,
          reinterpret_cast<UInt32>(str),
          length
        );
      }

      /**
       * Writes a string followed by a newline.
       * @param str
       *   Null-terminated string to write.
       */
      static inline void WriteLine(CString str) {
        if (!str) {
          str = "";
        }

        UInt32 length = StringLength(str);

        ABI::InvokeSystemCall(
          ABI::SystemCall::Console_WriteLine,
          reinterpret_cast<UInt32>(str),
          length
        );
      }

    private:
      /**
       * Calculates the length of a null-terminated string.
       * @param str
       *   Null-terminated string.
       * @return
       *   Length of the string in bytes, excluding the null terminator.
       */
      static inline UInt32 StringLength(CString str) {
        if (!str) {
          return 0;
        }

        UInt32 len = 0;

        while (str[len] != '\0') {
          ++len;
        }

        return len;
      }
  };
}
