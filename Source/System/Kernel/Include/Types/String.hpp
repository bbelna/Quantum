//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Types/String.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Simple string view class for kernel use.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>

/**
 * Simple string view class for kernel use.
 */
namespace Quantum::Kernel::Types {
  class String {
    public:
      constexpr String(CString data)
        : _data(data), _length(ComputeLength(data)) {}

      constexpr String(CString data, Size length)
        : _data(data), _length(length) {}

      /**
       * Gets the underlying C-style string data.
       * @return The C-style string.
       */
      constexpr const char* Data() const { return _data; }

      /**
       * Gets the length of the string.
       * @return The string length.
       */
      constexpr Size Length() const { return _length; }

      constexpr explicit operator bool() const {
        return _data != nullptr && _length != 0;
      }

      constexpr operator CString() const {
        return _data;
      }

    private:
      CString _data;
      Size _length;

      static constexpr Size ComputeLength(CString s) {
        Size count = 0;
        while (s[count] != '\0') count++;
        return count;
      }
  };
}
