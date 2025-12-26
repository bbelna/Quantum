/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Helpers/CStringHelper.hpp
 * C-string helper utilities.
 */

#include "Helpers/CStringHelper.hpp"

namespace Quantum::System::Kernel::Helpers {
  bool CStringHelper::WriteIntToBuffer(
    Int32 value,
    CStringMutable buffer,
    Size length
  ) {
    if (length == 0) {
      return false;
    }

    // handle sign
    bool negative = value < 0;
    UInt32 magnitude = negative
      ? static_cast<UInt32>(-static_cast<Int64>(value))
      : static_cast<UInt32>(value);
    char temp[_bufferSize] = {};
    Size idx = 0;
    Size out = 0;

    do {
      temp[idx++] = static_cast<char>('0' + (magnitude % 10));
      magnitude /= 10;
    } while (magnitude > 0 && idx < _bufferSize - 1);

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

  bool CStringHelper::AppendChar(
    CStringMutable buffer,
    Size length,
    Size& out,
    char c
  ) {
    if (!buffer || length == 0) {
      return false;
    } else if (out + 1 >= length) {
      // no room for this char + null terminator
      return false;
    } else {
      buffer[out++] = c;

      return true;
    }
  }

  bool CStringHelper::AppendString(
    CStringMutable buffer,
    Size length,
    Size& out,
    CString str
  ) {
    if (!buffer || length == 0) {
      return false;
    }

    if (!str) {
      str = "(null)";
    }

    for (Size i = 0; str[i] != '\0'; ++i) {
      if (!AppendChar(buffer, length, out, str[i])) {
        return false;
      }
    }

    return true;
  }

  bool CStringHelper::AppendUnsigned(
    CStringMutable buffer,
    Size length,
    Size& out,
    UInt32 value,
    UInt32 base,
    bool prefixHex
  ) {
    if (base < 2 || base > 16) {
      return false;
    }

    char temp[16] = {};
    Size idx = 0;
    CString digits = "0123456789ABCDEF";

    do {
      temp[idx++] = digits[value % base];
      value /= base;
    } while (value > 0 && idx < sizeof(temp));

    if (prefixHex) {
      if (!AppendString(buffer, length, out, "0x")) {
        return false;
      }
    }

    while (idx > 0) {
      if (!AppendChar(buffer, length, out, temp[--idx])) {
        return false;
      }
    }

    return true;
  }

  bool CStringHelper::ToCString(
    Int32 value,
    CStringMutable buffer,
    Size length
  ) {
    return WriteIntToBuffer(value, buffer, length);
  }

  char* CStringHelper::ToCString(Int32 value) {
    WriteIntToBuffer(value, _staticBuffer, _bufferSize);

    return _staticBuffer;
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
    auto append = [&](CString src) -> bool {
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

  bool CStringHelper::Concat(
    CString left,
    CString right,
    CStringMutable buffer
  ) {
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

  bool CStringHelper::Format(
    CStringMutable buffer,
    Size length,
    CString format,
    VariableArgumentsList args
  ) {
    if (!buffer || length == 0) {
      return false;
    }

    buffer[0] = '\0';

    if (!format) {
      return true;
    }

    Size out = 0;
    bool ok = true;

    for (CString p = format; *p != '\0'; ++p) {
      if (*p != '%') {
        if (!AppendChar(buffer, length, out, *p)) {
          ok = false;

          break;
        }

        continue;
      }

      // handle '%'
      ++p;

      if (*p == '\0') {
        break;
      }

      switch (*p) {
        case 's': {
          CString str = VARIABLE_ARGUMENTS(args, CString);

          if (!AppendString(buffer, length, out, str)) {
            ok = false;
          }

          break;
        }

        case 'c': {
          char c = static_cast<char>(VARIABLE_ARGUMENTS(args, int));

          if (!AppendChar(buffer, length, out, c)) {
            ok = false;
          }

          break;
        }

        case 'd': {
          Int32 v = VARIABLE_ARGUMENTS(args, Int32);
          char temp[_bufferSize] = {};

          if (!ToCString(v, temp, sizeof(temp))) {
            temp[0] = '\0';
          }

          if (!AppendString(buffer, length, out, temp)) {
            ok = false;
          }

          break;
        }

        case 'u': {
          UInt32 v = VARIABLE_ARGUMENTS(args, UInt32);

          if (!AppendUnsigned(buffer, length, out, v, 10, false)) {
            ok = false;
          }

          break;
        }

        case 'x': {
          UInt32 v = VARIABLE_ARGUMENTS(args, UInt32);

          if (!AppendUnsigned(buffer, length, out, v, 16, false)) {
            ok = false;
          }

          break;
        }

        case 'p': {
          UInt32 v = VARIABLE_ARGUMENTS(args, UInt32);

          if (!AppendUnsigned(buffer, length, out, v, 16, true)) {
            ok = false;
          }

          break;
        }

        case '%': {
          if (!AppendChar(buffer, length, out, '%')) {
            ok = false;
          }

          break;
        }

        default: {
          // unknown specifier: output it literally as "%x"
          if (
            !AppendChar(buffer, length, out, '%')
            || !AppendChar(buffer, length, out, *p)
          ) {
            ok = false;
          }

          break;
        }
      }

      if (!ok) {
        break;
      }
    }

    // always null-terminate
    if (out >= length) {
      buffer[length - 1] = '\0';
      return false;
    }

    buffer[out] = '\0';

    return ok;
  }
}
