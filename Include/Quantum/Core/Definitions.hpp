/**
 * @file Include/Quantum/Core/Definitions.hpp
 * @brief Commonly used compiler definitions and macros.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

/**
 * @brief Checks if a type is trivial.
 * @param T The type to check.
 */
#define IS_TRIVIAL(T) __is_trivial(T)

/**
 * @brief Checks if a type is trivially constructible.
 * @param T The type to check.
 */
#define IS_TRIVIALLY_CONSTRUCTIBLE(T) __is_trivially_constructible(T)

/**
 * @brief Checks if a type is trivially destructible.
 * @param T The type to check.
 */
#define IS_TRIVIALLY_DESTRUCTIBLE(T)  __is_trivially_destructible(T)

/**
 * @brief Checks if a type is trivially copyable.
 * @param T The type to check.
 */
#define IS_TRIVIALLY_COPYABLE(T) __is_trivially_copyable(T)

/**
 * @brief Checks if a type is standard layout.
 * @param T The type to check.
 */
#define IS_STANDARD_LAYOUT(T) __is_standard_layout(T)

/**
 * @brief Checks if a type is a POD (plain old data) type.
 * @param T The type to check.
 */
#define IS_POD(T) __is_pod(T)

/**
 * @brief Starts processing a variable arguments list.
 * @param list The variable arguments list.
 * @param last The last named parameter before the variable arguments.
 */
#define VARIABLE_ARGUMENTS_START(list, last) __builtin_va_start(list, last)

/**
 * @brief Ends processing a variable arguments list.
 * @param list The variable arguments list.
 */
#define VARIABLE_ARGUMENTS_END(list) __builtin_va_end(list)

/**
 * @brief Retrieves the next argument from a variable arguments list.
 * @param list The variable arguments list.
 * @param type The type of the argument to retrieve.
 */
#define VARIABLE_ARGUMENTS(list, type) __builtin_va_arg(list, type)

/**
 * @brief Checks if a bit flag is set in a value.
 * @param value The value to check.
 * @param bit The bit flag to check for.
 */
#define HAS_BIT(value, bit) (((value) & (bit)) != 0)

/**
 * @brief Kilobyte size in bytes.
 */
#define KB 1024u

/**
 * @brief Megabyte size in bytes.
 */
#define MB (1024u * KB)
