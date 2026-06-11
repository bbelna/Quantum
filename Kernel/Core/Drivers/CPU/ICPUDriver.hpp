/**
 * @file Kernel/Drivers/CPU/ICPUDriver.hpp
 * @brief Declares @ref @QKrnl::Drivers::CPU::ICPUDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Drivers/DriverTypes.hpp>
#include <KernelTypes.hpp>

#include "CPUDriverTypes.hpp"

namespace Quantum::Kernel::Drivers::CPU {
  /**
   * @brief Abstract CPU driver interface.
   *
   * Defines the architecture-independent contract for all CPU operations
   * including control flow, interrupt management, I/O port access, atomic
   * operations, memory barriers, paging, MSR access, and CPU feature
   * detection.
   */
  class ICPUDriver : public IDriver {
    public:
      /**
       * @brief Gets the name of the architecture as a string.
       * @return The architecture name string.
       */
      virtual const char* GetArchName() = 0;

      /**
       * @brief Halts the CPU until the next interrupt.
       */
      virtual void Halt() = 0;

      /**
       * @brief Halts the CPU forever. This function does not return.
       */
      [[noreturn]] virtual void HaltForever() = 0;

      /**
       * @brief Pauses the CPU (spin-wait hint).
       */
      virtual void Pause() = 0;

      /**
       * @brief Triggers a software-initiated scheduler yield.
       */
      virtual void Yield() = 0;

      /**
       * @brief Disables CPU interrupts.
       */
      virtual void DisableInterrupts() = 0;

      /**
       * @brief Enables CPU interrupts.
       */
      virtual void EnableInterrupts() = 0;

      /**
       * @brief Saves the current interrupt state and disables interrupts.
       * @return The saved flags value (for restoring later).
       */
      virtual UInt32 SaveAndDisableInterrupts() = 0;

      /**
       * @brief Restores the interrupt state from a previously saved value.
       * @param flags The saved flags value from
       *              @ref SaveAndDisableInterrupts.
       */
      virtual void RestoreInterrupts(UInt32 flags) = 0;

      /**
       * @brief Inputs a byte from the specified port.
       * @param port The I/O port.
       * @return The byte value read from the port.
       */
      virtual UInt8 In8(UInt16 port) = 0;

      /**
       * @brief Inputs a word from the specified port.
       * @param port The I/O port.
       * @return The word value read from the port.
       */
      virtual UInt16 In16(UInt16 port) = 0;

      /**
       * @brief Inputs a double word from the specified port.
       * @param port The I/O port.
       * @return The double word value read from the port.
       */
      virtual UInt32 In32(UInt16 port) = 0;

      /**
       * @brief Outputs a byte to the specified port.
       * @param port The I/O port.
       * @param value The byte value to output.
       */
      virtual void Out8(UInt16 port, UInt8 value) = 0;

      /**
       * @brief Outputs a word to the specified port.
       * @param port The I/O port.
       * @param value The word value to output.
       */
      virtual void Out16(UInt16 port, UInt16 value) = 0;

      /**
       * @brief Outputs a double word to the specified port.
       * @param port The I/O port.
       * @param value The double word value to output.
       */
      virtual void Out32(UInt16 port, UInt32 value) = 0;

      /**
       * @brief Atomically exchanges a 32-bit value at the given memory
       *        location.
       * @param pointer Pointer to the memory location.
       * @param value The value to exchange.
       * @return The old value at the memory location.
       */
      virtual UInt32 Exchange32(
        volatile UInt32* pointer,
        UInt32 value
      ) = 0;

      /**
       * @brief Atomically compares and exchanges a 32-bit value at the
       *        given memory location.
       * @param pointer Pointer to the memory location.
       * @param expected Reference to the expected value. Updated with the
       *                 actual value if the exchange fails.
       * @param desired The value to set if the current value matches
       *                @p expected.
       * @return `true` if the exchange was successful; `false` otherwise.
       */
      virtual bool CompareExchange32(
        volatile UInt32* pointer,
        UInt32& expected,
        UInt32 desired
      ) = 0;

      /**
       * @brief Atomically compares and exchanges a 64-bit value at the
       *        given memory location.
       * @param pointer Pointer to the memory location.
       * @param expected Reference to the expected value. Updated with the
       *                 actual value if the exchange fails.
       * @param desired The value to set if the current value matches
       *                @p expected.
       * @return `true` if the exchange was successful; `false` otherwise.
       */
      virtual bool CompareExchange64(
        volatile UInt64* pointer,
        UInt64& expected,
        UInt64 desired
      ) = 0;

      /**
       * @brief Atomically adds a delta to a 32-bit value at the given
       *        memory location.
       * @param pointer Pointer to the memory location.
       * @param delta The value to add.
       * @return The old value at the memory location before the addition.
       */
      virtual UInt32 FetchAdd32(
        volatile UInt32* pointer,
        UInt32 delta
      ) = 0;
  };
}
