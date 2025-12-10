//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Helpers/String.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// C-string helper utilities.
//------------------------------------------------------------------------------

#include <Helpers/CStringHelper.hpp>

namespace Quantum::Kernel::Helpers {
  namespace {
    constexpr Size bufferSize = 12;
    static char staticBuffer[bufferSize] = {};

    bool WriteIntToBuffer(Int32 value, CStringMutable buffer, Size length) {
      if (length == 0) {
        return false;
      }

      // handle sign
      bool negative = value < 0;
      UInt32 magnitude = negative
        ? static_cast<UInt32>(-static_cast<Int64>(value))
        : static_cast<UInt32>(value);
      char temp[bufferSize] = {};
      Size idx = 0;
      Size out = 0;

      do {
        temp[idx++] = static_cast<char>('0' + (magnitude % 10));
        magnitude /= 10;
      } while (magnitude > 0 && idx < bufferSize - 1);

      // ensure buffer has space for sign + digits + null
      Size needed = idx + (negative ? 1 : 0) + 1;

      if (needed > length) {
        return false;
      }

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

  bool CStringHelper::ToCString(Int32 value, CStringMutable buffer, Size length) {
    return WriteIntToBuffer(value, buffer, length);
  }

  char* CStringHelper::ToCString(Int32 value) {
    WriteIntToBuffer(value, staticBuffer, bufferSize);
    return staticBuffer;
  }

  Size CStringHelper::Length(CString str) {
    Size len = 0;
    if (!str) {
      return 0;
    }
    while (str[len] != '\0') {
      ++len;
    }
    return len;
  }

  bool CStringHelper::Concat(
    CString left,
    CString right,
    CStringMutable buffer,
    Size length
  ) {
    if (!buffer || length == 0) {
      return false;
    }

    buffer[0] = '\0';

    Size out = 0;
    auto append = [&](const char* src) -> bool {
      if (!src) {
        return true;
      }

      for (Size i = 0; src[i] != '\0'; ++i) {
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

  bool CStringHelper::Concat(CString left, CString right, CStringMutable buffer) {
    if (!buffer) {
      return false;
    }

    Size leftLen = Length(left);
    Size rightLen = Length(right);

    // include null terminator
    Size total = leftLen + rightLen + 1;

    if (total == 0) {
      return false;
    }

    return Concat(left, right, buffer, total);
  }
}
