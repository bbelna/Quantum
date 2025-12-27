/**
 * @file System/Kernel/Include/String.hpp
 * @brief Simple immutable string view class for kernel use.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Types.hpp"

namespace Quantum::System::Kernel {
  /**
   * Simple immutable string view class for kernel use.
   */
  class String {
    public:
      /**
       * Constructs a string view from a null-terminated C string.
       * @param data
       *   Pointer to the C string (may be `nullptr`).
       */
      constexpr String(CString data)
        : _data(data), _length(ComputeLength(data)) {}

      /**
       * Constructs a string view from a buffer with explicit length.
       * @param data
       *   Pointer to the buffer (may be `nullptr`).
       * @param length
       *   Length of the buffer in bytes.
       */
      constexpr String(CString data, Size length)
        : _data(data), _length(length) {}

      /**
       * Gets the underlying C-style string data.
       * @return
       *   The C-style string.
       */
      constexpr CString Data() const { return _data; }

      /**
       * Gets the length of the string.
       * @return
       *   The string length.
       */
      constexpr Size Length() const { return _length; }

      constexpr explicit operator bool() const {
        return _data != nullptr && _length != 0;
      }

      constexpr operator CString() const {
        return _data;
      }

    private:
      /**
       * Pointer to the underlying character buffer.
       */
      CString _data;

      /**
       * Length of the buffer in bytes.
       */
      Size _length;

      /**
       * Computes the length of a null-terminated C string.
       * @param s
       *   Pointer to the C string.
       * @return
       *   Length in bytes excluding the terminator.
       */
      static constexpr Size ComputeLength(CString s) {
        Size count = 0;
        while (s[count] != '\0') count++;
        return count;
      }
  };
}
