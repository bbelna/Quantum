/**
 * @file System/Kernel/Include/Arch/IA32/IO.hpp
 * @brief IA32 port I/O operations.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 port I/O primitives.
   */
  class IO {
    public:
      /**
       * Inputs a byte from the specified port.
       * @param port
       *   The I/O port.
       * @return
       *   The byte value read from the port.
       */
      static UInt8 In8(UInt16 port);

      /**
       * Inputs a word from the specified port.
       * @param port
       *   The I/O port.
       * @return
       *   The word value read from the port.
       */
      static UInt16 In16(UInt16 port);

      /**
       * Inputs a double word from the specified port.
       * @param port
       *   The I/O port.
       * @return
       *   The double word value read from the port.
       */
      static UInt32 In32(UInt16 port);

      /**
       * Outputs a byte to the specified port.
       * @param port
       *   The I/O port.
       * @param value
       *   The byte value to output.
       */
      static void Out8(UInt16 port, UInt8 value);

      /**
       * Outputs a word to the specified port.
       * @param port
       *   The I/O port.
       * @param value
       *   The word value to output.
       */
      static void Out16(UInt16 port, UInt16 value);

      /**
       * Outputs a double word to the specified port.
       * @param port
       *   The I/O port.
       * @param value
       *   The double word value to output.
       */
      static void Out32(UInt16 port, UInt32 value);
  };
}
