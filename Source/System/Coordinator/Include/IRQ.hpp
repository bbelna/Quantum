/**
 * @file System/Coordinator/Include/IRQ.hpp
 * @brief Coordinator IRQ routing.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <ABI/SystemCall.hpp>
#include <Types.hpp>

namespace Quantum::System::Coordinator {
  /**
   * Coordinator IRQ routing.
   */
  class IRQ {
    public:
      /**
       * Initializes the IRQ routing port.
       */
      static void Initialize();

      /**
       * Processes any pending IRQ routing messages.
       */
      static void ProcessPending();

    private:
      /**
       * IRQ routing port id.
       */
      inline static UInt32 _portId = 0;

      /**
       * IRQ routing port handle.
       */
      inline static UInt32 _portHandle = 0;

      /**
       * Registers an IRQ routing port.
       * @param irq
       *   IRQ line number.
       * @param portId
       *   IPC port owned by the caller.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Register(UInt32 irq, UInt32 portId) {
        return ABI::InvokeSystemCall(ABI::SystemCall::IRQ_Register, irq, portId);
      }

      /**
       * Unregisters an IRQ routing port.
       * @param irq
       *   IRQ line number.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Unregister(UInt32 irq) {
        return ABI::InvokeSystemCall(ABI::SystemCall::IRQ_Unregister, irq);
      }

      /**
       * Enables an IRQ line.
       * @param irq
       *   IRQ line number.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Enable(UInt32 irq) {
        return ABI::InvokeSystemCall(ABI::SystemCall::IRQ_Enable, irq);
      }

      /**
       * Disables an IRQ line.
       * @param irq
       *   IRQ line number.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Disable(UInt32 irq) {
        return ABI::InvokeSystemCall(ABI::SystemCall::IRQ_Disable, irq);
      }
  };
}
