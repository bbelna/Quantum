/**
 * @file Core/ConfigurationParser.cpp
 * @brief Implements @ref @QCore::ConfigurationParser.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "ConfigurationParser.hpp"

namespace Quantum::Core {
  namespace {
    /**
     * @brief Returns whether @p character is a horizontal whitespace
     *        byte (space or tab).
     */
    bool IsHorizontalWhitespace(char character) {
      return character == ' ' || character == '\t';
    }

    /**
     * @brief Returns whether @p character terminates a logical line.
     */
    bool IsLineTerminator(char character) {
      return
        character == '\n' ||
        character == '\r' ||
        character == '\0';
    }

    /**
     * @brief Compares two null-terminated C-strings for equality
     *        without depending on the @ref @QCore::CString helpers.
     */
    bool StringsEqual(const char* left, const char* right) {
      if (left == nullptr || right == nullptr) {
        return left == right;
      }

      Size index = 0;

      for (;;) {
        char leftCharacter = left[index];
        char rightCharacter = right[index];

        if (leftCharacter != rightCharacter) {
          return false;
        }

        if (leftCharacter == '\0') {
          return true;
        }

        ++index;
      }
    }
  }

  bool ConfigurationParser::Parse(char* buffer, Size length) {
    _entryCount = 0;

    if (buffer == nullptr) {
      return false;
    }

    Size cursor = 0;

    while (
      cursor < length &&
      _entryCount < MaxEntries
    ) {
      // skip leading horizontal whitespace
      while (
        cursor < length &&
        IsHorizontalWhitespace(buffer[cursor])
      ) {
        ++cursor;
      }

      // skip blank lines and comments
      if (
        cursor < length &&
        (
          buffer[cursor] == '\n' ||
          buffer[cursor] == '\r' ||
          buffer[cursor] == '#' ||
          buffer[cursor] == ';'
        )
      ) {
        // advance to the next line terminator
        while (
          cursor < length &&
          buffer[cursor] != '\n'
        ) {
          ++cursor;
        }

        if (cursor < length) {
          ++cursor;
        }

        continue;
      }

      if (cursor >= length) {
        break;
      }

      // record the key start
      Size keyStart = cursor;
      Size keyEnd = cursor;

      // walk to the '=' separator (or end of line)
      while (
        cursor < length &&
        buffer[cursor] != '=' &&
        !IsLineTerminator(buffer[cursor])
      ) {
        // track the last non-whitespace position so we can right-trim
        if (!IsHorizontalWhitespace(buffer[cursor])) {
          keyEnd = cursor + 1;
        }

        ++cursor;
      }

      bool hasSeparator = cursor < length && buffer[cursor] == '=';

      if (hasSeparator) {
        // null-terminate the trimmed key
        if (keyEnd < length) {
          buffer[keyEnd] = '\0';
        }

        // step past the '='
        ++cursor;

        // skip leading horizontal whitespace in value
        while (
          cursor < length &&
          IsHorizontalWhitespace(buffer[cursor])
        ) {
          ++cursor;
        }

        Size valueStart = cursor;
        Size valueEnd = cursor;

        // walk to end of line
        while (
          cursor < length &&
          !IsLineTerminator(buffer[cursor])
        ) {
          if (!IsHorizontalWhitespace(buffer[cursor])) {
            valueEnd = cursor + 1;
          }

          ++cursor;
        }

        // null-terminate the trimmed value
        if (valueEnd < length) {
          buffer[valueEnd] = '\0';
        }

        // record the entry only if the key has at least one byte
        // and fits within MaxKeyLength / MaxValueLength (including
        // the null terminator)
        Size keyLength = keyEnd - keyStart;
        Size valueLength = valueEnd - valueStart;

        if (
          keyLength > 0 &&
          keyLength + 1 <= MaxKeyLength &&
          valueLength + 1 <= MaxValueLength
        ) {
          _entries[_entryCount].Key = &buffer[keyStart];
          _entries[_entryCount].Value = &buffer[valueStart];
          ++_entryCount;
        }
      } else {
        // no '=' on this line - skip the rest of it
        while (
          cursor < length &&
          buffer[cursor] != '\n'
        ) {
          ++cursor;
        }
      }

      // consume the line terminator(s)
      while (
        cursor < length &&
        (
          buffer[cursor] == '\n' ||
          buffer[cursor] == '\r'
        )
      ) {
        ++cursor;
      }
    }

    return true;
  }

  Size ConfigurationParser::GetEntryCount() const {
    return _entryCount;
  }

  const char* ConfigurationParser::GetKey(Size index) const {
    if (index < _entryCount) {
      return _entries[index].Key;
    }

    return nullptr;
  }

  const char* ConfigurationParser::GetValue(Size index) const {
    if (index < _entryCount) {
      return _entries[index].Value;
    }

    return nullptr;
  }

  const char* ConfigurationParser::Find(const char* key) const {
    if (key == nullptr) {
      return nullptr;
    }

    for (
      Size index = 0;
      index < _entryCount;
      ++index
    ) {
      if (StringsEqual(_entries[index].Key, key)) {
        return _entries[index].Value;
      }
    }

    return nullptr;
  }
}
