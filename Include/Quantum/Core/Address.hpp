/**
 * @file Include/Quantum/Core/Address.hpp
 * @brief Declaration of address types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Types.hpp"

namespace Quantum::Core {
  /**
   * @brief Represents an address.
   * @tparam ValueType The underlying type used to store the raw address value.
   *                   Defaults to `UIntPtr`.
   */
  template <typename ValueType = UIntPtr>
  class Address {
    public:
      /**
       * @brief Constructs a null address.
       */
      constexpr Address() : Value(0) {}

      /**
       * @brief Constructs an address from a raw address value.
       * @param address The raw address value.
       */
      constexpr explicit Address(ValueType address) : Value(address) {}

      /**
       * @brief Retrieves the raw address value.
       * @return The raw address value.
       */
      constexpr ValueType Raw() const { return Value; }

      /**
       * @brief Checks if the address is null.
       * @return `true` if the address is null; `false` otherwise.
       */
      constexpr bool IsNull() const { return Value == 0; }

      /**
       * @brief Converts the address to a `ValueType`.
       * @return The address as a `ValueType`.
       */
      constexpr explicit operator ValueType() const { return Value; }

      /**
       * @brief Compares two addresses for equality.
       * @param lhs The left-hand side address.
       * @param rhs The right-hand side address.
       * @return `true` if the two addresses are equal; `false` otherwise.
       */
      friend constexpr bool operator==(Address lhs, Address rhs) {
        return lhs == rhs;
      }

      /**
       * @brief Compares two addresses for inequality.
       * @param lhs The left-hand side address.
       * @param rhs The right-hand side address.
       * @return `true` if the two addresses are not equal; `false` otherwise.
       */
      friend constexpr bool operator!=(Address lhs, Address rhs) {
        return lhs != rhs;
      }

      /**
       * @brief Compares if one address is less than another.
       * @param lhs The left-hand side address.
       * @param rhs The right-hand side address.
       * @return
       *   `true` if the left-hand side address is less than the right-hand
       *   side; `false` otherwise.
       */
      friend constexpr bool operator<(Address lhs, Address rhs) {
        return lhs < rhs;
      }

      /**
       * @brief Compares if one address is less than or equal to another.
       * @param lhs The left-hand side address.
       * @param rhs The right-hand side address.
       * @return
       *   `true` if the left-hand side address is less than or equal to the
       *   right-hand side; `false` otherwise.
       */
      friend constexpr bool operator<=(Address lhs, Address rhs) {
        return lhs <= rhs;
      }

      /**
       * @brief Compares if one address is greater than another.
       * @param lhs The left-hand side address.
       * @param rhs The right-hand side address.
       * @return
       *   `true` if the left-hand side address is greater than the right-hand
       *   side; `false` otherwise.
       */
      friend constexpr bool operator>(Address lhs, Address rhs) {
        return lhs > rhs;
      }

      /**
       * @brief Compares if one address is greater than or equal to another.
       * @param lhs The left-hand side address.
       * @param rhs The right-hand side address.
       * @return
       *   `true` if the left-hand side address is greater than or equal to the
       *   right-hand side; `false` otherwise.
       */
      friend constexpr bool operator>=(Address lhs, Address rhs) {
        return lhs >= rhs;
      }

      /**
       * @brief Adds an offset to an address.
       * @param address The address.
       * @param offset The offset to add.
       * @return The resulting address after addition.
       */
      friend constexpr Address operator+(
        Address address,
        UIntPtr offset
      ) {
        return Address(address + offset);
      }

      /**
       * @brief Subtracts an offset from an address.
       * @param address The address.
       * @param offset The offset to subtract.
       * @return The resulting address after subtraction.
       */
      friend constexpr Address operator-(
        Address address,
        UIntPtr offset
      ) {
        return Address(address - offset);
      }

      /**
       * @brief Calculates the difference between two addresses.
       * @param lhs The left-hand side address.
       * @param rhs The right-hand side address.
       * @return The difference between the two addresses.
       */
      friend constexpr ValueType operator-(Address lhs, Address rhs) {
        return lhs - rhs;
      }

    private:
      /**
       * @brief The raw address value.
       */
      ValueType Value;
  };

  /**
   * @brief Represents a block of addresses.
   * @tparam ValueType The underlying type used to store the raw address value.
   *                   Defaults to @ref UIntPtr.
   */
  template <typename ValueType = UIntPtr>
  struct AddressBlock {
    /**
     * @brief Base address of the block.
     */
    ValueType Base = 0;

    /**
     * @brief Size in bytes of the block.
     */
    Size SizeInBytes = 0;

    /**
     * @brief Compares two address blocks by their base address.
     * @param lhs The left-hand side block.
     * @param rhs The right-hand side block.
     * @return `true` if the left block's base is less than the right's.
     */
    friend constexpr bool operator<(
      const AddressBlock& lhs,
      const AddressBlock& rhs
    ) {
      return lhs.Base < rhs.Base;
    }
  };

  /**
   * @brief Represents a generic memory block.
   */
  struct MemoryBlock : public AddressBlock<> {};
}
