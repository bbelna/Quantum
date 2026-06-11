/**
 * @file Kernel/Memory/Stack.hpp
 * @brief Declares @ref @QKrnl::Memory::Stack.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Memory {
  /**
   * @brief Represents a stack in memory.
   * @see <a href="https://en.wikipedia.org/wiki/Stack_(abstract_data_type)">
   *        Wikipedia: Stack (abstract data type)
   *      </a>
   */
  struct Stack : public MemoryBlock {
    /**
     * @brief The address of the top of the stack.
     */
    UIntPtr Top;

    /**
     * @brief Pushes a value onto the stack.
     * @tparam ValueType The type of the value to push.
     * @param value The value to push onto the stack.
     * @return `true` if the value was successfully pushed; `false` otherwise.
     */
    template <typename ValueType>
    bool Push(ValueType value) {
      if (Top - sizeof(ValueType) >= Base) {
        Top = Top - sizeof(ValueType);
        *reinterpret_cast<ValueType*>(Top) = value;

        return true;
      } else {
        return false;
      }
    }

    /**
     * @brief Pushes a block of raw bytes onto the stack.
     * @param data Pointer to the source data.
     * @param size Number of bytes to push.
     * @return `true` if the data was pushed; `false` otherwise.
     */
    bool PushBytes(
      const void* data,
      Size size
    ) {
      if (Top - size >= Base) {
        Top = Top - size;

        const UInt8* source = static_cast<const UInt8*>(data);
        UInt8* destination = reinterpret_cast<UInt8*>(Top);

        for (Size i = 0; i < size; ++i) {
          destination[i] = source[i];
        }

        return true;
      } else {
        return false;
      }
    }
  };
}
