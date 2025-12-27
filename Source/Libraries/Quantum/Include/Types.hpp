/**
 * @file Libraries/Quantum/Include/Types.hpp
 * @brief Declaration of primitive types.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#define VARIABLE_ARGUMENTS_START(list, last) \
  ((list) = (VariableArgumentsList)(&last + 1))
#define VARIABLE_ARGUMENTS_END(list)
#define VARIABLE_ARGUMENTS(list, type) \
  (list += sizeof(type), *(type *)(list - sizeof(type)))

typedef unsigned char UInt8;
typedef unsigned short UInt16;
typedef unsigned int UInt32;
typedef unsigned long long UInt64;

typedef signed char Int8;
typedef signed short Int16;
typedef signed int Int32;
typedef signed long long Int64;

typedef UInt32 UIntPtr;
typedef Int32 IntPtr;
typedef UInt32 Size;

typedef const char* CString;
typedef char* CStringMutable;

typedef char* VariableArgumentsList;
