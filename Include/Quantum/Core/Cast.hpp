/**
 * @file Include/Quantum/Core/Cast.hpp
 * @brief Declares @ref @QCore::Cast.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

/**
 * @brief Type-safe pointer cast utilities.
 *
 * Provides @ref As overloads for converting between pointer types without
 * scattering raw C++ casts throughout the codebase. `void*` overloads use
 * `static_cast`; typed-to-typed overloads use `reinterpret_cast`.
 */
namespace Quantum::Core::Cast {
  /**
   * @brief Casts an opaque `const void*` to a typed `const T*`.
   * @tparam T The target type.
   * @param pointer The opaque pointer to cast.
   * @return A `const T*` pointing to the same address.
   *
   * Uses `static_cast`, which is well-defined when @p pointer originally
   * referred to a `T` (or a layout-compatible type).
   */
  template<typename T>
  constexpr const T* As(const void* pointer) noexcept {
    return static_cast<const T*>(pointer);
  }

  /**
   * @brief Casts an opaque `void*` to a typed `T*`.
   * @tparam T The target type.
   * @param pointer The opaque pointer to cast.
   * @return A `T*` pointing to the same address.
   *
   * Uses `static_cast`, which is well-defined when @p pointer originally
   * referred to a `T` (or a layout-compatible type).
   */
  template<typename T>
  constexpr T* As(void* pointer) noexcept {
    return static_cast<T*>(pointer);
  }

  /**
   * @brief Casts a typed `const From*` to a `const T*`.
   * @tparam T The target type.
   * @tparam From The source type (deduced from @p pointer).
   * @param pointer The typed pointer to cast.
   * @return A `const T*` pointing to the same address.
   *
   * Uses `reinterpret_cast`. The caller is responsible for ensuring the
   * resulting pointer is only dereferenced when the underlying object is
   * layout-compatible with `T`.
   */
  template<typename T, typename From>
  constexpr const T* As(const From* pointer) noexcept {
    return reinterpret_cast<const T*>(pointer);
  }

  /**
   * @brief Casts a typed `From*` to a `T*`.
   * @tparam T The target type.
   * @tparam From The source type (deduced from @p pointer).
   * @param pointer The typed pointer to cast.
   * @return A `T*` pointing to the same address.
   *
   * Uses `reinterpret_cast`. The caller is responsible for ensuring the
   * resulting pointer is only dereferenced when the underlying object is
   * layout-compatible with `T`.
   */
  template<typename T, typename From>
  constexpr T* As(From* pointer) noexcept {
    return reinterpret_cast<T*>(pointer);
  }
}
