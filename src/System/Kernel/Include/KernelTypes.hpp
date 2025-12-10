//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/KernelTypes.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Declaration of types used throughout the kernel.
//------------------------------------------------------------------------------

#pragma once

#define VARIABLE_ARGUMENTS_START(list, last) ((list) = (VariableArgumentsList)(&last + 1))
#define VARIABLE_ARGUMENTS_END(list)
#define VARIABLE_ARGUMENTS(list, type) (list += sizeof(type), *(type *)(list - sizeof(type)))

typedef unsigned char UInt8;
typedef unsigned short UInt16;
typedef unsigned int UInt32;
typedef unsigned long long UInt64;

typedef signed char Int8;
typedef signed short Int16;
typedef signed int Int32;
typedef signed long long Int64;

typedef UInt32 UIntPtr;
typedef Int32  IntPtr;
typedef UInt32 Size;

typedef const char* CString;
typedef char* CStringMutable;

typedef char* VariableArgumentsList;

/**
 * Simple string view class for kernel use.
 */
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
