/**
 * @file Core/CString.cpp
 * @brief Implements @ref @QCore::CString.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "CString.hpp"
#include "Definitions.hpp"

namespace Quantum::Core::CString {
  namespace {
    /**
     * @brief Size of the static buffer for integer to C-string conversions.
     */
    constexpr Size IntBufferSize = 12;

    /**
     * @brief Size of the static buffer for hexadecimal integer to C-string
     *        conversions (8 digits + `"0x"` prefix + null terminator).
     */
    constexpr Size HexIntBufferSize = 16;

    /**
     * @brief Appends a character to a buffer.
     * @param buffer Destination buffer.
     * @param length Size of the destination buffer.
     * @param out Current output position (updated on success).
     * @param c Character to append.
     * @return `true` on success; `false` if the buffer is too small.
     */
    bool AppendCharacter(char* buffer, Size length, Size& out, char c) {
      if (!buffer || length == 0) {
        return false;
      } else if (out + 1 > length) {
        // no room for this char + null terminator
        return false;
      } else {
        buffer[out++] = c;

        return true;
      }
    }

    /**
     * @brief Appends a C-string to a buffer.
     * @param buffer Destination buffer.
     * @param length Size of the destination buffer.
     * @param out Current output position (updated on success).
     * @param str C-string to append.
     * @return `true` on success; `false` if the buffer is too small.
     */
    bool AppendString(
      char* buffer,
      Size length,
      Size& out,
      const char* str
    ) {
      if (!buffer || length == 0) {
        return false;
      } else if (!str) {
        str = "(null)";
      }

      for (Size i = 0; str[i] != '\0'; ++i) {
        if (!AppendCharacter(buffer, length, out, str[i])) {
          return false;
        }
      }

      return true;
    }

    /**
     * @brief Appends an unsigned integer to a buffer as a string in a specified
     *        base.
     * @param buffer Destination buffer.
     * @param length Size of the destination buffer.
     * @param out Current output position (updated on success).
     * @param value Unsigned integer to append.
     * @param base Numeric base (e.g., 10 for decimal, 16 for hexadecimal).
     * @param prefixHex Whether to prefix hexadecimal values with `"0x"`.
     * @return `true` on success; `false` if the buffer is too small.
     */
    bool AppendUnsigned(
      char* buffer,
      Size length,
      Size& out,
      UInt32 value,
      UInt32 base,
      bool prefixHex
    ) {
      if (base < 2 || base > 16) return false;

      char intBuffer[HexIntBufferSize] = {};
      Size index = 0;
      const char* digits = "0123456789ABCDEF";

      do {
        intBuffer[index++] = digits[value % base];
        value /= base;
      } while (value > 0 && index < sizeof(intBuffer));

      if (prefixHex) {
        if (!AppendString(buffer, length, out, "0x")) {
          return false;
        }
      }

      while (index > 0) {
        if (!AppendCharacter(buffer, length, out, intBuffer[--index])) {
          return false;
        }
      }

      return true;
    }

    /**
     * @brief Writes a signed integer to a buffer as a decimal string.
     * @param value Integer to write.
     * @param buffer Destination buffer.
     * @param length Size of the destination buffer.
     * @return `true` on success; `false` if the buffer is too small.
     */
    bool WriteIntToBuffer(Int32 value, char* buffer, Size length) {
      if (length == 0) return false;

      // handle sign
      bool negative = value < 0;
      char intBuffer[IntBufferSize] = {};
      Size index = 0;
      Size out = 0;
      UInt32 magnitude = negative
        ? static_cast<UInt32>(-static_cast<Int64>(value))
        : static_cast<UInt32>(value);

      do {
        intBuffer[index++] = static_cast<char>('0' + (magnitude % 10));
        magnitude /= 10;
      } while (magnitude > 0 && index < IntBufferSize - 1);

      // ensure buffer has space for sign + digits + null
      Size needed = index + (negative ? 1 : 0) + 1;

      if (needed > length) {
        return false;
      } else if (negative) {
        buffer[out++] = '-';
      }

      while (index > 0) {
        buffer[out++] = intBuffer[--index];
      }

      buffer[out] = '\0';

      return true;
    }
  }

  bool ToCString(Int32 value, char* buffer, Size length) {
    return WriteIntToBuffer(value, buffer, length);
  }

  Size Length(const char* str) {
    Size len = 0;

    if (!str) return 0;

    while (str[len] != '\0') ++len;

    return len;
  }

  Size Length(const char* str, Size maxLength) {
    if (!str) return 0;

    Size length = 0;

    while (length < maxLength && str[length] != '\0') ++length;

    return length;
  }

  bool Concat(
    const char* left,
    const char* right,
    char* buffer,
    Size length
  ) {
    if (!buffer || length == 0) return false;

    buffer[0] = '\0';

    Size out = 0;

    auto append = [&](const char* src) -> bool {
      if (!src) return true;

      for (Size i = 0; src[i] != '\0'; ++i) {
        if (out + 1 > length) {
          buffer[out] = '\0';

          return false;
        }

        buffer[out++] = src[i];
      }

      return true;
    };

    if (!append(left) || !append(right)) return false;

    buffer[out] = '\0';

    return true;
  }

  bool Concat(const char* left, const char* right, char* buffer) {
    if (!buffer) return false;

    Size leftLength = Length(left);
    Size rightLength = Length(right);
    Size total = leftLength + rightLength + 1;

    if (total == 0) return false;

    return Concat(left, right, buffer, total);
  }

  bool FormatV(
    char* buffer,
    Size length,
    const char* format,
    VariableArgumentsList args
  ) {
    if (!buffer || length == 0) return false;

    buffer[0] = '\0';

    if (!format) return true;

    Size out = 0;
    bool ok = true;

    for (const char* p = format; *p != '\0'; ++p) {
      if (*p != '%') {
        if (!AppendCharacter(buffer, length, out, *p)) {
          ok = false;

          break;
        }

        continue;
      }

      // start of format specifier
      ++p;

      if (*p == '\0') break;

      // parse flags
      bool flagLeftAlign = false;
      bool flagZeroPad = false;
      bool flagSpace = false;
      bool flagPlus = false;
      bool flagHash = false;

      for (;;) {
        if (*p == '-') {
          flagLeftAlign = true;
        } else if (*p == '0') {
          flagZeroPad = true;
        } else if (*p == ' ') {
          flagSpace = true;
        } else if (*p == '+') {
          flagPlus = true;
        } else if (*p == '#') {
          flagHash = true;
        } else {
          break;
        }

        ++p;
      }

      // left-align overrides zero-pad
      if (flagLeftAlign) flagZeroPad = false;

      // parse width (number or '*')
      Int32 width = 0;
      bool hasWidth = false;

      if (*p == '*') {
        width = VARIABLE_ARGUMENTS(args, int);
        hasWidth = true;

        if (width < 0) {
          flagLeftAlign = true;
          width = -width;
        }

        ++p;
      } else {
        while (*p >= '0' && *p <= '9') {
          width = width * 10 + (*p - '0');
          hasWidth = true;
          ++p;
        }
      }

      // parse precision (`.` followed by number or `*`)
      Int32 precision = -1;

      if (*p == '.') {
        ++p;

        if (*p == '*') {
          precision = VARIABLE_ARGUMENTS(args, int);

          if (precision < 0) precision = 0;

          ++p;
        } else {
          precision = 0;

          while (*p >= '0' && *p <= '9') {
            precision = precision * 10 + (*p - '0');
            ++p;
          }
        }
      }

      // skip length modifiers ('l', 'll', 'h', 'hh') — all integer
      // arguments are promoted to at least 32 bits by variadic promotion;
      // 64-bit ('ll') is not supported on this 32-bit freestanding target
      // to avoid pulling in __udivmoddi4
      if (*p == 'l') {
        ++p;

        if (*p == 'l') ++p;
      } else if (*p == 'h') {
        ++p;

        if (*p == 'h') ++p;
      }

      if (*p == '\0') break;

      // dispatch on conversion specifier
      switch (*p) {
        case 's': {
          const char* str = VARIABLE_ARGUMENTS(args, const char*);

          if (!str) str = "(null)";

          // compute string length, capped by precision
          Size stringLength = 0;

          if (precision >= 0) {
            while (stringLength < static_cast<Size>(precision) &&
                   str[stringLength] != '\0') {
              ++stringLength;
            }
          } else {
            while (str[stringLength] != '\0') ++stringLength;
          }

          // right-pad if needed
          Size padAmount = (hasWidth && static_cast<Size>(width) > stringLength)
            ? static_cast<Size>(width) - stringLength
            : 0;

          if (!flagLeftAlign) {
            for (Size i = 0; i < padAmount; ++i) {
              if (!AppendCharacter(buffer, length, out, ' ')) {
                ok = false;

                break;
              }
            }
          }

          if (ok) {
            for (Size i = 0; i < stringLength; ++i) {
              if (!AppendCharacter(buffer, length, out, str[i])) {
                ok = false;

                break;
              }
            }
          }

          if (ok && flagLeftAlign) {
            for (Size i = 0; i < padAmount; ++i) {
              if (!AppendCharacter(buffer, length, out, ' ')) {
                ok = false;

                break;
              }
            }
          }

          break;
        }

        case 'c': {
          char c = static_cast<char>(VARIABLE_ARGUMENTS(args, int));

          if (!AppendCharacter(buffer, length, out, c)) ok = false;

          break;
        }

        case 'd':
        case 'i': {
          // determine sign and magnitude; use 32-bit arithmetic when
          // possible to avoid pulling in __udivmoddi4
          char signChar = '\0';
          char digitBuffer[24] = {};
          Size digitCount = 0;

          {
            Int32 value = VARIABLE_ARGUMENTS(args, Int32);

            if (value < 0) {
              signChar = '-';
              value = -value;
            } else if (flagPlus) {
              signChar = '+';
            } else if (flagSpace) {
              signChar = ' ';
            }

            UInt32 magnitude = static_cast<UInt32>(value);

            do {
              digitBuffer[digitCount++]
                = static_cast<char>('0' + magnitude % 10);
              magnitude /= 10;
            } while (magnitude > 0 && digitCount < sizeof(digitBuffer));
          }

          // apply precision (minimum digits)
          Size minDigits = (precision > 0) ? static_cast<Size>(precision) : 1;

          while (digitCount < minDigits && digitCount < sizeof(digitBuffer)) {
            digitBuffer[digitCount++] = '0';
          }

          // zero-precision with value 0 produces no digits
          if (precision == 0 && digitCount == 1 && digitBuffer[0] == '0') {
            digitCount = 0;
          }

          Size totalLength = digitCount + (signChar ? 1 : 0);
          Size padAmount = (hasWidth && static_cast<Size>(width) > totalLength)
            ? static_cast<Size>(width) - totalLength
            : 0;

          // emit: [pad] [sign] [zeros/digits] or [sign] [zero-pad] [digits]
          if (!flagLeftAlign && !flagZeroPad) {
            for (Size i = 0; i < padAmount; ++i) {
              if (!AppendCharacter(buffer, length, out, ' ')) {
                ok = false;

                break;
              }
            }
          }

          if (ok && signChar) {
            if (!AppendCharacter(buffer, length, out, signChar)) ok = false;
          }

          if (ok && !flagLeftAlign && flagZeroPad) {
            for (Size i = 0; i < padAmount; ++i) {
              if (!AppendCharacter(buffer, length, out, '0')) {
                ok = false;

                break;
              }
            }
          }

          if (ok) {
            Size i = digitCount;

            while (i > 0) {
              if (!AppendCharacter(buffer, length, out, digitBuffer[--i])) {
                ok = false;

                break;
              }
            }
          }

          if (ok && flagLeftAlign) {
            for (Size i = 0; i < padAmount; ++i) {
              if (!AppendCharacter(buffer, length, out, ' ')) {
                ok = false;

                break;
              }
            }
          }

          break;
        }

        case 'u':
        case 'x':
        case 'X':
        case 'o': {
          UInt32 base = 10;
          bool upperHex = (*p == 'X');

          if (*p == 'x' || *p == 'X') {
            base = 16;
          } else if (*p == 'o') {
            base = 8;
          }

          const char* digits = upperHex
            ? "0123456789ABCDEF"
            : "0123456789abcdef";

          // render digits into temp buffer (reversed)
          char digitBuffer[24] = {};
          Size digitCount = 0;

          {
            UInt32 value = VARIABLE_ARGUMENTS(args, UInt32);

            do {
              digitBuffer[digitCount++] = digits[value % base];
              value /= base;
            } while (value > 0 && digitCount < sizeof(digitBuffer));
          }

          // apply precision (minimum digits)
          Size minDigits = (precision > 0) ? static_cast<Size>(precision) : 1;

          while (digitCount < minDigits && digitCount < sizeof(digitBuffer)) {
            digitBuffer[digitCount++] = '0';
          }

          if (precision == 0 && digitCount == 1 && digitBuffer[0] == '0') {
            digitCount = 0;
          }

          // hash prefix: "0x"/"0X" for hex, "0" for octal
          const char* hashPrefix = "";
          Size hashLength = 0;

          if (flagHash && digitCount > 0) {
            if (base == 16) {
              hashPrefix = upperHex ? "0X" : "0x";
              hashLength = 2;
            } else if (base == 8 && digitBuffer[digitCount - 1] != '0') {
              hashPrefix = "0";
              hashLength = 1;
            }
          }

          Size totalLength = digitCount + hashLength;
          Size padAmount = (hasWidth && static_cast<Size>(width) > totalLength)
            ? static_cast<Size>(width) - totalLength
            : 0;

          if (!flagLeftAlign && !flagZeroPad) {
            for (Size i = 0; i < padAmount; ++i) {
              if (!AppendCharacter(buffer, length, out, ' ')) {
                ok = false;

                break;
              }
            }
          }

          if (ok) {
            for (Size i = 0; i < hashLength; ++i) {
              if (!AppendCharacter(buffer, length, out, hashPrefix[i])) {
                ok = false;

                break;
              }
            }
          }

          if (ok && !flagLeftAlign && flagZeroPad) {
            for (Size i = 0; i < padAmount; ++i) {
              if (!AppendCharacter(buffer, length, out, '0')) {
                ok = false;

                break;
              }
            }
          }

          if (ok) {
            Size i = digitCount;

            while (i > 0) {
              if (!AppendCharacter(buffer, length, out, digitBuffer[--i])) {
                ok = false;

                break;
              }
            }
          }

          if (ok && flagLeftAlign) {
            for (Size i = 0; i < padAmount; ++i) {
              if (!AppendCharacter(buffer, length, out, ' ')) {
                ok = false;

                break;
              }
            }
          }

          break;
        }

        case 'p': {
          UInt32 v = VARIABLE_ARGUMENTS(args, UInt32);

          if (!AppendUnsigned(buffer, length, out, v, 16, true)) ok = false;

          break;
        }

        case '%': {
          if (!AppendCharacter(buffer, length, out, '%')) ok = false;

          break;
        }

        default: {
          if (
            !AppendCharacter(buffer, length, out, '%') ||
            !AppendCharacter(buffer, length, out, *p)
          ) ok = false;

          break;
        }
      }

      if (!ok) break;
    }

    // always null-terminate
    if (out >= length) {
      buffer[length - 1] = '\0';

      return false;
    }

    buffer[out] = '\0';

    return ok;
  }

  bool Format(
    char* buffer,
    Size length,
    const char* format,
    ...
  ) {
    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, format);

    bool result = FormatV(buffer, length, format, args);

    VARIABLE_ARGUMENTS_END(args);

    return result;
  }

  bool ConcatAndFormatV(
    const char* left,
    const char* right,
    char* buffer,
    Size length,
    VariableArgumentsList args
  ) {
    char concatBuffer[FormatBufferSize] = {};

    if (!Concat(left, right, concatBuffer, sizeof(concatBuffer))) return false;

    bool result = FormatV(buffer, length, concatBuffer, args);

    return result;
  }

  bool ConcatAndFormat(
    const char* left,
    const char* right,
    char* buffer,
    Size length,
    ...
  ) {
    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, length);

    bool result = ConcatAndFormatV(left, right, buffer, length, args);

    VARIABLE_ARGUMENTS_END(args);

    return result;
  }

  UInt32 Copy(
    const char* source,
    UInt8* destination,
    UInt32 maxBytes
  ) {
    if (!source || !destination || maxBytes == 0) return 0;

    UInt32 length = 0;

    while (length + 1 < maxBytes && source[length] != '\0') {
      destination[length] = static_cast<UInt8>(source[length]);
      ++length;
    }

    destination[length] = '\0';

    return length + 1;
  }

  
  UInt32 Copy(
    const char* source,
    char* destination,
    UInt32 maxBytes
  ) {
    if (!source || !destination || maxBytes == 0) return 0;

    UInt32 length = 0;

    while (length + 1 < maxBytes && source[length] != '\0') {
      destination[length] = source[length];
      ++length;
    }

    destination[length] = '\0';

    return length + 1;
  }

  Int32 Compare(const char* left, const char* right) {
    if (!left && !right) return 0;
    if (!left) return -1;
    if (!right) return 1;

    while (*left && *right) {
      if (*left != *right) {
        return static_cast<Int32>(
          static_cast<UInt8>(*left)
        ) - static_cast<Int32>(
          static_cast<UInt8>(*right)
        );
      }

      ++left;
      ++right;
    }

    return static_cast<Int32>(
      static_cast<UInt8>(*left)
    ) - static_cast<Int32>(
      static_cast<UInt8>(*right)
    );
  }

  bool Equals(const char* left, const char* right, UInt32 length) {
    if (!left || !right) return false;

    for (UInt32 i = 0; i < length; ++i) {
      if (left[i] != right[i]) return false;
    }

    return true;
  }

  bool Equals(const char* left, const char* right) {
    if (!left || !right) return false;

    UInt32 i = 0;

    while (left[i] != '\0' && right[i] != '\0') {
      if (left[i] != right[i]) return false;

      ++i;
    }

    return left[i] == right[i];
  }

  void Set(void* destination, UInt8 value, Size count) {
    if (!destination || count == 0) return;

    UInt8* ptr = reinterpret_cast<UInt8*>(destination);

    for (Size i = 0; i < count; ++i) ptr[i] = value;
  }

  bool StartsWith(const char* str, const char* prefix) {
    if (!str || !prefix) return false;

    UInt32 i = 0;

    while (prefix[i] != '\0') {
      if (str[i] != prefix[i]) return false;

      ++i;
    }

    return true;
  }

  Size PadRight(
    const char* str,
    char* buffer,
    Size bufferSize,
    Size fieldWidth,
    char fill
  ) {
    if (!buffer || bufferSize == 0) return 0;

    Size stringLength = str ? Length(str) : 0;
    Size writeLength = fieldWidth < bufferSize - 1
      ? fieldWidth
      : bufferSize - 1;

    Size copyLength = stringLength < writeLength
      ? stringLength
      : writeLength;

    for (Size index = 0; index < copyLength; ++index) {
      buffer[index] = str[index];
    }

    for (Size index = copyLength; index < writeLength; ++index) {
      buffer[index] = fill;
    }

    buffer[writeLength] = '\0';

    return writeLength;
  }

  Size PadLeft(
    const char* str,
    char* buffer,
    Size bufferSize,
    Size fieldWidth,
    char fill
  ) {
    if (!buffer || bufferSize == 0) return 0;

    Size stringLength = str ? Length(str) : 0;
    Size writeLength = fieldWidth < bufferSize - 1
      ? fieldWidth
      : bufferSize - 1;

    if (stringLength >= writeLength) {
      // string is wider than the field; just copy what fits
      for (Size index = 0; index < writeLength; ++index) {
        buffer[index] = str[index];
      }
    } else {
      Size paddingLength = writeLength - stringLength;

      for (Size index = 0; index < paddingLength; ++index) {
        buffer[index] = fill;
      }

      for (Size index = 0; index < stringLength; ++index) {
        buffer[paddingLength + index] = str[index];
      }
    }

    buffer[writeLength] = '\0';

    return writeLength;
  }
}
