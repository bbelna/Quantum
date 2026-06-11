/**
 * @file Include/Quantum/Core/Value.hpp
 * @brief Declaration of utilities for value wrappers.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::Core {
  /**
   * @brief Wrapper class for a value.
   * @tparam ValueType The type of the value.
   */
  template <class ValueType>
  class Value {
    public:
      /**
       * @brief Creates a new `Value` with a default-initialized value.
       */
      constexpr Value() = default;

      /**
       * @brief Creates a new `Value` with the given value.
       * @param value The initial value.
       */
      constexpr Value(const ValueType& value) : _value(value) {}

      /**
       * @brief Creates a new `Value` with the given value.
       * @param value The initial value.
       */
      constexpr Value(
        ValueType&& value
      ) noexcept : _value(static_cast<ValueType&&>(value)) {}

      /**
       * @brief Assigns a new value.
       * @param value The new value.
       * @return Reference to this `Value`.
       */
      constexpr Value& operator=(const ValueType& value) { 
        _value = value;

        return *this;
      }

      /**
       * @brief Assigns a new value.
       * @param value The new value.
       * @return Reference to this `Value`.
       */
      constexpr Value& operator=(ValueType&& value) noexcept {
        _value = static_cast<ValueType&&>(value);

        return *this;
      }

      /**
       * @brief Conversion operator to the underlying value type.
       */
      constexpr operator ValueType&() & noexcept { return _value; }

      /**
       * @brief Conversion operator to the underlying value type.
       */
      constexpr operator const ValueType&() const & noexcept { return _value; }

      /**
       * @brief Conversion operator to the underlying value type.
       */
      constexpr operator ValueType&&() && noexcept {
        return static_cast<ValueType&&>(_value);
      }

      /**
       * @brief Pointer access operator.
       * @return Pointer to the underlying value.
       */
      constexpr ValueType* operator->() noexcept { return &_value; }

      /**
       * @brief Pointer access operator.
       * @return Pointer to the underlying value.
       */
      constexpr const ValueType* operator->() const noexcept { return &_value; }

      /**
       * @brief Dereference operator.
       * @return Reference to the underlying value.
       */
      constexpr ValueType& operator*() noexcept { return _value; }

      /**
       * @brief Dereference operator.
       * @return Reference to the underlying value.
       */
      constexpr const ValueType& operator*() const noexcept { return _value; }

      /**
       * @brief Gets the underlying value.
       * @return The underlying value.
       */
      constexpr ValueType& get() & noexcept { return _value; }

      /**
       * @brief Gets the underlying value.
       * @return The underlying value.
       */
      constexpr const ValueType& get() const & noexcept { return _value; }

      /**
       * @brief Gets the underlying value.
       * @return The underlying value.
       */
      constexpr ValueType&& get() && noexcept {
        return static_cast<ValueType&&>(_value);
      }

    private:
      /**
       * @brief The stored value.
       */
      ValueType _value;
  };

  /**
   * @brief Base class for objects that have a value.
   * @tparam ValueType The type of the value.
   */
  template <typename ValueType>
  class HasValue {
    public:
      /**
       * @brief Creates a new @ref HasValue instance with the given value.
       * @param value The initial value.
       */
      HasValue(ValueType value) : _value(value) {}

      /**
       * @brief Destroys this @ref HasValue instance.
       */
      virtual ~HasValue() = default;

      /**
       * @brief Gets the value.
       * @return The value.
       */
      virtual ValueType GetValue() const { return this->_value; }

    protected:
      /**
       * @brief The stored value.
       */
      ValueType _value;
  };

  /**
   * @brief Base class for objects that have a mutable value.
   * @tparam ValueType The type of the value.
   */
  template <typename ValueType>
  class HasMutableValue : public HasValue<ValueType> {
    public:
      /**
       * @brief Creates a new @ref HasMutableValue instance with the given
       *        value.
       * @param value The initial value.
       */
      HasMutableValue(ValueType value) : HasValue<ValueType>(value) {}

      /**
       * @brief Destroys this @ref HasMutableValue instance.
       */
      virtual ~HasMutableValue() = default;

      /**
       * @brief Sets the value.
       * @param value The new value.
       */
      virtual void SetValue(ValueType value) { this->_value = value; }
  };
}

namespace Quantum {
  using namespace Core;
}
