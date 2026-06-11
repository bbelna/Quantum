/**
 * @file Include/Quantum/Core/Enum.hpp
 * @brief Declares @ref @QCore::Enum.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

/**
 * @brief Enum bitmask utilities and type traits.
 */
namespace Quantum::Core::Enum {
  /**
   * @brief Maps an enum type to its underlying integral type.
   * @tparam E The enum type.
   */
  template<typename E>
  struct UnderlyingType {
    using Type = __underlying_type(E);
  };

  /**
   * @brief Casts an enum value to its underlying integral type.
   * @tparam E The enum type.
   * @param e The enum value to cast.
   * @return The underlying integral value.
   */
  template<typename E>
  constexpr auto ToBase(E e) noexcept -> typename UnderlyingType<E>::Type {
    return static_cast<typename UnderlyingType<E>::Type>(e);
  }

  /**
   * @brief Casts an integral value to the specified enum type.
   * @tparam E The target enum type.
   * @param value The integral value to cast.
   * @return The corresponding enum value.
   */
  template<typename E>
  constexpr E To(typename UnderlyingType<E>::Type value) noexcept {
    return static_cast<E>(value);
  }

  /**
   * @brief
   *   Type trait that controls whether bitmask operators are enabled for an
   *   enum type. Specialize with `Value = true` to opt in, or use
   *   @ref QUANTUM_ENABLE_ENUM_BITMASK_OPS.
   * @tparam E The enum type.
   */
  template<typename E>
  struct EnableBitMaskOps {
    static constexpr bool Value = false;
  };

  /**
   * @brief
   *   Concept satisfied by enum types that have opted into bitmask operations
   *   via @ref EnableBitMaskOps.
   * @tparam E The enum type.
   */
  template<typename E>
  concept BitMaskEnum = __is_enum(E) && EnableBitMaskOps<E>::Value;

  /**
   * @brief Tests whether all bits in `flag` are set in `value`.
   * @tparam E A @ref BitMaskEnum type.
   * @param value The value to test.
   * @param flag The flag bits to check for.
   * @return `true` if every bit in `flag` is set in `value`.
   */
  template<BitMaskEnum E>
  constexpr bool HasFlag(E value, E flag) noexcept {
    return (ToBase(value) & ToBase(flag)) == ToBase(flag);
  }

  /**
   * @brief Tests whether any bit in `flag` is set in `value`.
   * @tparam E A @ref BitMaskEnum type.
   * @param value The value to test.
   * @param flag The flag bits to check for.
   * @return `true` if at least one bit in `flag` is set in `value`.
   */
  template<BitMaskEnum E>
  constexpr bool HasAnyFlag(E value, E flag) noexcept {
    return (ToBase(value) & ToBase(flag)) != 0;
  }

  /**
   * @brief Tests whether a bitmask value has no bits set.
   * @tparam E A @ref BitMaskEnum type.
   * @param value The value to test.
   * @return `true` if `value` is zero.
   */
  template<BitMaskEnum E>
  constexpr bool IsNone(E value) noexcept {
    return ToBase(value) == 0;
  }
}

/**
 * @brief
 *   Opts an enum type into bitmask operators and injects them into the
 *   enum's namespace so ADL can find them.
 * @param NS The fully-qualified namespace containing the enum.
 * @param E  The unqualified enum type name.
 */
#define QUANTUM_ENABLE_ENUM_BITMASK_OPS(NS, E) \
  template<> \
  struct Quantum::Core::Enum::EnableBitMaskOps<NS::E> { \
    static constexpr bool Value = true; \
  }; \
  \
  namespace NS { \
    constexpr E operator|(E lhs, E rhs) noexcept { \
      return static_cast<E>( \
        Quantum::Core::Enum::ToBase(lhs) \
        | Quantum::Core::Enum::ToBase(rhs) \
      ); \
    } \
    \
    constexpr E operator&(E lhs, E rhs) noexcept { \
      return static_cast<E>( \
        Quantum::Core::Enum::ToBase(lhs) \
        & Quantum::Core::Enum::ToBase(rhs) \
      ); \
    } \
    \
    constexpr E operator^(E lhs, E rhs) noexcept { \
      return static_cast<E>( \
        Quantum::Core::Enum::ToBase(lhs) \
        ^ Quantum::Core::Enum::ToBase(rhs) \
      ); \
    } \
    \
    constexpr E operator~(E value) noexcept { \
      return static_cast<E>(~Quantum::Core::Enum::ToBase(value)); \
    } \
    \
    constexpr E& operator|=(E& lhs, E rhs) noexcept { \
      lhs = lhs | rhs; \
      \
      return lhs; \
    } \
    \
    constexpr E& operator&=(E& lhs, E rhs) noexcept { \
      lhs = lhs & rhs; \
      \
      return lhs; \
    } \
    \
    constexpr E& operator^=(E& lhs, E rhs) noexcept { \
      lhs = lhs ^ rhs; \
      \
      return lhs; \
    } \
    \
    constexpr auto operator|( \
      typename Quantum::Core::Enum::UnderlyingType<E>::Type lhs, \
      E rhs \
    ) noexcept -> typename Quantum::Core::Enum::UnderlyingType<E>::Type { \
      return lhs | Quantum::Core::Enum::ToBase(rhs); \
    } \
    \
    constexpr auto operator|( \
      E lhs, \
      typename Quantum::Core::Enum::UnderlyingType<E>::Type rhs \
    ) noexcept -> typename Quantum::Core::Enum::UnderlyingType<E>::Type { \
      return Quantum::Core::Enum::ToBase(lhs) | rhs; \
    } \
    \
    constexpr bool operator==( \
      typename Quantum::Core::Enum::UnderlyingType<E>::Type lhs, \
      E rhs \
    ) noexcept { \
      return lhs == Quantum::Core::Enum::ToBase(rhs); \
    } \
    \
    constexpr bool operator!=( \
      typename Quantum::Core::Enum::UnderlyingType<E>::Type lhs, \
      E rhs \
    ) noexcept { \
      return lhs != Quantum::Core::Enum::ToBase(rhs); \
    } \
  }

/**
 * @brief
 *   Opts an enum type into comparison and mixed-type operators for
 *   value/ordinal enums (as opposed to bitmask enums).
 * @param NS The fully-qualified namespace containing the enum.
 * @param E  The unqualified enum type name.
 */
#define QUANTUM_ENABLE_ENUM_VALUE_OPS(NS, E) \
  namespace NS { \
    using _Base_##E = Quantum::Core::Enum::UnderlyingType<E>::Type; \
    \
    constexpr bool operator<(E a, E b) noexcept { \
      return Quantum::Core::Enum::ToBase(a) \
           < Quantum::Core::Enum::ToBase(b); \
    } \
    \
    constexpr bool operator<=(E a, E b) noexcept { \
      return Quantum::Core::Enum::ToBase(a) \
          <= Quantum::Core::Enum::ToBase(b); \
    } \
    \
    constexpr bool operator>(E a, E b) noexcept { \
      return Quantum::Core::Enum::ToBase(a) \
           > Quantum::Core::Enum::ToBase(b); \
    } \
    \
    constexpr bool operator>=(E a, E b) noexcept { \
      return Quantum::Core::Enum::ToBase(a) \
          >= Quantum::Core::Enum::ToBase(b); \
    } \
    \
    constexpr bool operator==(_Base_##E a, E b) noexcept { \
      return a == Quantum::Core::Enum::ToBase(b); \
    } \
    \
    constexpr bool operator!=(_Base_##E a, E b) noexcept { \
      return a != Quantum::Core::Enum::ToBase(b); \
    } \
    \
    constexpr bool operator<(_Base_##E a, E b) noexcept { \
      return a < Quantum::Core::Enum::ToBase(b); \
    } \
    \
    constexpr bool operator<=(_Base_##E a, E b) noexcept { \
      return a <= Quantum::Core::Enum::ToBase(b); \
    } \
    \
    constexpr bool operator>(_Base_##E a, E b) noexcept { \
      return a > Quantum::Core::Enum::ToBase(b); \
    } \
    \
    constexpr bool operator>=(_Base_##E a, E b) noexcept { \
      return a >= Quantum::Core::Enum::ToBase(b); \
    } \
    \
    constexpr bool operator==(E a, _Base_##E b) noexcept { \
      return Quantum::Core::Enum::ToBase(a) == b; \
    } \
    \
    constexpr bool operator!=(E a, _Base_##E b) noexcept { \
      return Quantum::Core::Enum::ToBase(a) != b; \
    } \
    \
    constexpr bool operator<(E a, _Base_##E b) noexcept { \
      return Quantum::Core::Enum::ToBase(a) < b; \
    } \
    \
    constexpr bool operator<=(E a, _Base_##E b) noexcept { \
      return Quantum::Core::Enum::ToBase(a) <= b; \
    } \
    \
    constexpr bool operator>(E a, _Base_##E b) noexcept { \
      return Quantum::Core::Enum::ToBase(a) > b; \
    } \
    \
    constexpr bool operator>=(E a, _Base_##E b) noexcept { \
      return Quantum::Core::Enum::ToBase(a) >= b; \
    } \
    \
    constexpr E& operator++(E& e) noexcept { \
      e = static_cast<E>(Quantum::Core::Enum::ToBase(e) + 1); \
      \
      return e; \
    } \
    \
    constexpr E operator++(E& e, int) noexcept { \
      E old = e; \
      \
      e = static_cast<E>(Quantum::Core::Enum::ToBase(e) + 1); \
      \
      return old; \
    } \
    \
    constexpr E& operator--(E& e) noexcept { \
      e = static_cast<E>(Quantum::Core::Enum::ToBase(e) - 1); \
      \
      return e; \
    } \
    \
    constexpr E operator--(E& e, int) noexcept { \
      E old = e; \
      \
      e = static_cast<E>(Quantum::Core::Enum::ToBase(e) - 1); \
      \
      return old; \
    } \
  }
