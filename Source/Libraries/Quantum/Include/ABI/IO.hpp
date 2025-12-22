/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/ABI/IO.hpp
 * Port I/O syscall wrappers.
 */

#pragma once

#include <ABI/SystemCall.hpp>
#include <Types.hpp>

namespace Quantum::ABI {
  /**
   * Port I/O syscall wrappers.
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
      static UInt8 In8(UInt16 port) {
        return static_cast<UInt8>(InvokeSystemCall(
          SystemCall::IO_In8,
          port
        ));
      }

      /**
       * Inputs a word from the specified port.
       * @param port
       *   The I/O port.
       * @return
       *   The word value read from the port.
       */
      static UInt16 In16(UInt16 port) {
        return static_cast<UInt16>(InvokeSystemCall(
          SystemCall::IO_In16,
          port
        ));
      }

      /**
       * Inputs a double word from the specified port.
       * @param port
       *   The I/O port.
       * @return
       *   The double word value read from the port.
       */
      static UInt32 In32(UInt16 port) {
        return InvokeSystemCall(
          SystemCall::IO_In32,
          port
        );
      }

      /** 
       * Outputs a byte to the specified port.
       * @param port
       *   The I/O port.
       * @param value
       *   The byte value to output.
       * @return
       *   The result of the system call (0 on success, 1 on failure).
       */
      static UInt32 Out8(UInt16 port, UInt8 value) {
        return InvokeSystemCall(
          SystemCall::IO_Out8,
          port,
          value
        );
      }

      /**
       * Outputs a word to the specified port.
       * @param port
       *   The I/O port.
       * @param value
       *   The word value to output.
       * @return
       *   The result of the system call (0 on success, 1 on failure).
       */
      static UInt32 Out16(UInt16 port, UInt16 value) {
        return InvokeSystemCall(
          SystemCall::IO_Out16,
          port,
          value
        );
      }

      /**
       * Outputs a double word to the specified port.
       * @param port
       *   The I/O port.
       * @param value
       *   The double word value to output.
       * @return
       *   The result of the system call (0 on success, 1 on failure).
       */
      static UInt32 Out32(UInt16 port, UInt32 value) {
        return InvokeSystemCall(
          SystemCall::IO_Out32,
          port,
          value
        );
      }

      /**
       * Grants the specified task I/O port access.
       * @param taskId
       *   The ID of the task to grant I/O access to.
       * @return
       *   The result of the system call (0 on success, 1 on failure).
       */
      static UInt32 GrantIOAccess(UInt32 taskId) {
        return InvokeSystemCall(
          SystemCall::Task_GrantIOAccess,
          taskId
        );
      }
  };
}
