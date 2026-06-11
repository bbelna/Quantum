/**
 * @file Include/Quantum/Core/ConfigurationParser.hpp
 * @brief Declares @ref @QCore::ConfigurationParser.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Types.hpp"

namespace Quantum::Core {
  /**
   * @brief Parses simple line-oriented `KEY=VALUE` configuration files.
   *
   * The parser operates *in place* on a caller-owned buffer: newlines,
   * `'='` separators, and trailing whitespace are overwritten with
   * null terminators so that key/value pointers can alias the buffer
   * directly. The buffer must therefore remain valid for as long as
   * any @ref ConfigurationParser instance refers to it.
   *
   * Lines beginning with `'#'` or `';'` (after leading whitespace) and
   * blank lines are skipped. Leading and trailing spaces/tabs around
   * keys and values are trimmed. Carriage returns (`'\r'`) are tolerated
   * for files with CRLF line endings.
   *
   * Lines without a `'='` separator are silently ignored. Keys longer
   * than @ref MaxKeyLength are truncated; values longer than
   * @ref MaxValueLength are truncated. Once @ref MaxEntries entries
   * have been parsed, additional lines are ignored.
   */
  class ConfigurationParser {
    public:
      /**
       * @brief Maximum number of entries the parser can hold.
       */
      static constexpr Size MaxEntries = 32;

      /**
       * @brief Maximum length of a key (including null terminator).
       */
      static constexpr Size MaxKeyLength = 64;

      /**
       * @brief Maximum length of a value (including null terminator).
       */
      static constexpr Size MaxValueLength = 256;

      /**
       * @brief Creates a new, empty @ref ConfigurationParser.
       */
      ConfigurationParser() = default;

      /**
       * @brief Parses a configuration buffer in place.
       * @param buffer
       *   Mutable pointer to the configuration text. The buffer is
       *   modified: separators and trailing whitespace are replaced
       *   with null terminators so that the stored key/value pointers
       *   can alias buffer storage directly.
       * @param length Number of bytes in @p buffer.
       * @return
       *   `true` if parsing completed (even if the file was empty or
       *   contained no valid entries); `false` only if @p buffer is
       *   `nullptr`.
       *
       * Calling @ref Parse a second time discards the previous result.
       */
      bool Parse(char* buffer, Size length);

      /**
       * @brief Returns the number of parsed entries.
       */
      Size GetEntryCount() const;

      /**
       * @brief Returns the key at @p index, or `nullptr` if out of range.
       */
      const char* GetKey(Size index) const;

      /**
       * @brief Returns the value at @p index, or `nullptr` if out of range.
       */
      const char* GetValue(Size index) const;

      /**
       * @brief Looks up a value by key (case sensitive).
       * @param key The key to look up.
       * @return
       *   Pointer to the parsed value, or `nullptr` if @p key is not
       *   present in the parsed configuration.
       */
      const char* Find(const char* key) const;

    private:
      /**
       * @brief A single parsed key/value pair, aliased into the
       *        caller's buffer.
       */
      struct Entry {
        /**
         * @brief Pointer to the null-terminated key.
         */
        const char* Key;

        /**
         * @brief Pointer to the null-terminated value.
         */
        const char* Value;
      };

      /**
       * @brief Storage for parsed entries.
       */
      Entry _entries[MaxEntries] = {};

      /**
       * @brief Number of valid entries in @ref _entries.
       */
      Size _entryCount = 0;
  };
}
