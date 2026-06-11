/**
 * @file Include/Quantum/Kernel/ABI/PortIO.hpp
 * @brief Declaration of the kernel port I/O ABI.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "ABI.hpp"

/**
 * @brief ABI functions for port I/O operations.
 */
namespace Quantum::Kernel::ABI::PortIO {
  /**
   * @brief Reads a byte from the specified I/O port.
   * @param port The I/O port address.
   * @return The byte read from the port.
   */
  inline UInt8 In8(UInt16 port) {
    return static_cast<UInt8>(Invoke(
      KernelOperation::PortIO_In8,
      static_cast<UInt32>(port),
      0,
      0
    ));
  }

  /**
   * @brief Reads a word from the specified I/O port.
   * @param port The I/O port address.
   * @return The word read from the port.
   */
  inline UInt16 In16(UInt16 port) {
    return static_cast<UInt16>(Invoke(
      KernelOperation::PortIO_In16,
      static_cast<UInt32>(port),
      0,
      0
    ));
  }

  /**
   * @brief Reads a double word from the specified I/O port.
   * @param port The I/O port address.
   * @return The double word read from the port.
   */
  inline UInt32 In32(UInt16 port) {
    return Invoke(
      KernelOperation::PortIO_In32,
      static_cast<UInt32>(port),
      0,
      0
    );
  }

  /**
   * @brief Writes a byte to the specified I/O port.
   * @param port The I/O port address.
   * @param value The byte to write.
   */
  inline void Out8(UInt16 port, UInt8 value) {
    Invoke(
      KernelOperation::PortIO_Out8,
      static_cast<UInt32>(port),
      static_cast<UInt32>(value),
      0
    );
  }

  /**
   * @brief Writes a word to the specified I/O port.
   * @param port The I/O port address.
   * @param value The word to write.
   */
  inline void Out16(UInt16 port, UInt16 value) {
    Invoke(
      KernelOperation::PortIO_Out16,
      static_cast<UInt32>(port),
      static_cast<UInt32>(value),
      0
    );
  }

  /**
   * @brief Writes a double word to the specified I/O port.
   * @param port The I/O port address.
   * @param value The double word to write.
   */
  inline void Out32(UInt16 port, UInt32 value) {
    Invoke(
      KernelOperation::PortIO_Out32,
      static_cast<UInt32>(port),
      value,
      0
    );
  }
}
