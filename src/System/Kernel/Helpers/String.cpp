//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Helpers/String.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Simple string helper utilities.
//------------------------------------------------------------------------------

#include <Helpers/String.hpp>

namespace Quantum::Kernel::Helpers {
  namespace {
    // Enough for 32-bit int min (-2147483648) plus null terminator.
    constexpr usize bufferSize = 12;

    bool WriteIntToBuffer(int32 value, char* buffer, usize length) {
      if (length == 0) {
        return false;
      }

      // handle sign
      bool negative = value < 0;
      uint32 magnitude = negative
        ? static_cast<uint32>(-static_cast<int64>(value))
        : static_cast<uint32>(value);

      // build digits in reverse
      char temp[bufferSize] = {};
      usize idx = 0;
      do {
        temp[idx++] = static_cast<char>('0' + (magnitude % 10));
        magnitude /= 10;
      } while (magnitude > 0 && idx < bufferSize - 1);

      // ensure buffer has space for sign + digits + null
      usize needed = idx + (negative ? 1 : 0) + 1;
      if (needed > length) {
        return false;
      }

      usize out = 0;
      if (negative) {
        buffer[out++] = '-';
      }

      while (idx > 0) {
        buffer[out++] = temp[--idx];
      }

      buffer[out] = '\0';
      return true;
    }
  }

  bool String::ToString(int32 value, char* buffer, usize length) {
    return WriteIntToBuffer(value, buffer, length);
  }

  usize String::Length(const char* str) {
    usize len = 0;
    if (!str) {
      return 0;
    }
    while (str[len] != '\0') {
      ++len;
    }
    return len;
  }

  bool String::Concat(
    const char* left,
    const char* right,
    char* buffer,
    usize length
  ) {
    if (!buffer || length == 0) {
      return false;
    }

    buffer[0] = '\0';

    usize out = 0;
    auto append = [&](const char* src) -> bool {
      if (!src) {
        return true;
      }

      for (usize i = 0; src[i] != '\0'; ++i) {
        if (out + 1 >= length) {
          buffer[out] = '\0';
          return false;
        }

        buffer[out++] = src[i];
      }

      return true;
    };

    if (!append(left)) {
      return false;
    } else if (!append(right)) {
      return false;
    }

    buffer[out] = '\0';
    return true;
  }

  bool String::Concat(const char* left, const char* right, char* buffer) {
    if (!buffer) {
      return false;
    }

    usize leftLen = Length(left);
    usize rightLen = Length(right);

    // include null terminator
    usize total = leftLen + rightLen + 1;

    if (total == 0) {
      return false;
    }

    return Concat(left, right, buffer, total);
  }
}
