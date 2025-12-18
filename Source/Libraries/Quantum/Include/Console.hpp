/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/Console.hpp
 * User-mode console wrapper.
 */

#pragma once

#include <ABI/InvokeSystemCall.hpp>
#include <ABI/Types/SystemCall.hpp>
#include <Types/Primitives.hpp>

namespace Quantum {
  namespace {
    inline UInt32 StringLength(const char* str) {
      if (!str) {
        return 0;
      }

      UInt32 len = 0;

      while (str[len] != '\0') {
        ++len;
      }

      return len;
    }
  }

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
      static inline void Write(const char* str) {
        if (!str) {
          return;
        }

        UInt32 length = StringLength(str);

        if (length == 0) {
          return;
        }

        ABI::InvokeSystemCall(
          ABI::Types::SystemCall::Write,
          reinterpret_cast<UInt32>(str),
          length
        );
      }

      /**
       * Writes a string followed by a newline.
       * @param str
       *   Null-terminated string to write.
       */
      static inline void WriteLine(const char* str) {
        Write(str);
        ABI::InvokeSystemCall(
          ABI::Types::SystemCall::Write,
          reinterpret_cast<UInt32>("\n"),
          1
        );
      }
  };
}
