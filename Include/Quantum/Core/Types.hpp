/**
 * @file Include/Quantum/Core/Types.hpp
 * @brief Declaration of primitive types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

/**
 * @brief Unsigned 8-bit integer.
 */
typedef unsigned char UInt8;

/**
 * @brief Unsigned 16-bit integer.
 */
typedef unsigned short UInt16;

/**
 * @brief Unsigned 32-bit integer.
 */
typedef unsigned int UInt32;

/**
 * @brief Unsigned 64-bit integer.
 */
typedef unsigned long long UInt64;

/**
 * @brief Signed 8-bit integer.
 */
typedef signed char Int8;

/**
 * @brief Signed 16-bit integer.
 */
typedef signed short Int16;

/**
 * @brief Signed 32-bit integer.
 */
typedef signed int Int32;

/**
 * @brief Signed 64-bit integer.
 */
typedef signed long long Int64;

/**
 * @brief Unsigned pointer-sized integer.
 */
typedef unsigned int UIntPtr;

/**
 * @brief Signed pointer-sized integer.
 */
typedef signed int IntPtr;

/**
 * @brief Size type.
 */
typedef unsigned int Size;

/**
 * @brief Variable arguments list type.
 */
typedef __builtin_va_list VariableArgumentsList;

constexpr UInt8 InvalidUInt8 = static_cast<UInt8>(-1);
