/**
 * @file System/Kernel/Include/Console.hpp
 * @brief Console output handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

#include "Logger.hpp"

namespace Quantum::System::Kernel {
  /**
   * Architecture-agnostic console driver.
   */
  class Console {
    public:
      /**
       * Initializes the active console backend.
       */
      static void Initialize();

      /**
       * Writes a null-terminated string to the console.
       * @param str
       *   The string to write.
       */
      static void Write(CString str);

      /**
       * Writes a buffer with the given length to the console.
       * @param string
       *   The string to write.
       * @param length
       *   The length of the string.
       */
      static void Write(CString string, UInt32 length);

      /**
       * Writes a string followed by a newline.
       * @param string
       *   The string to write.
       * @param length
       *   The length of the string.
       */
      static void WriteLine(CString string, UInt32 length);

      /**
       * Writes a line (null-terminated string followed by newline) to the
       * console.
       * @param string
       *   The string to write.
       */
      static void WriteLine(CString string);

      /**
       * Writes a formatted string to the console.
       * @param format
       *   Formatted string.
       * @param ...
       *   Format arguments.
       */
      static void WriteFormatted(CString format, ...);

      /**
       * Gets a logger writer that routes messages through the console.
       * @return
       *   The logger writer instance.
       */
      static Logger::Writer& GetWriter();

    private:
      /**
       * Logger writer adapter for console output.
       */
      class WriterAdapter : public Logger::Writer {
        public:
          /**
           * Writes a message.
           * @param message
           *   The message to write.
           */
          void Write(CString message) override;
      };

      /**
       * Flag indicating whether a write operation is in progress.
       */
      inline static volatile UInt32 _writing = 0;

      /**
       * Converts an unsigned integer to a string in the given base.
       * @param value
       *   Number to convert.
       * @param base
       *   Conversion base (10 or 16).
       * @param prefixHex
       *   Whether to prefix 0x for hex.
       */
      static void WriteUnsigned(
        UInt32 value,
        UInt32 base,
        bool prefixHex = false
      );

      /**
       * Writes a formatted string to the console using a variable argument
       * list.
       * @param format
       *   Format string.
       * @param args
       *   Variable argument list.
       */
      static void WriteFormattedVariableArguments(
        CString format,
        VariableArgumentsList args
      );

      /**
       * Writes a buffer with the given length to the console without locking.
       * @param buffer
       *   The string to write.
       * @param length
       *   The length of the string.
       */
      static void WriteUnlocked(CString string, UInt32 length);

      /**
       * Writes a buffer with the given length to the console without locking.
       * @param buffer
       *   The string to write.
       * @param length
       *   The length of the string.
       */
      static void WriteUnlocked(CString string);
  };
}
