/**
 * @file Kernel/Arch/IA32/Handlers/IA32MaydayHandler.hpp
 * @brief Declares @ref @QKrnlIA32::IA32MaydayHandler.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Handlers/IMaydayHandler.hpp>
#include <KernelTypes.hpp>

namespace Quantum::Kernel::Arch::IA32::Handlers {
  /**
   * @brief IA-32 implementation of @ref IMaydayHandler that logs
   *        architecture-specific diagnostic information and halts the system.
   *
   * Dumps the current `ESP`, a 16-dword stack dump, and walks the `EBP`
   * chain to produce a call trace before entering an infinite halt loop
   * with interrupts disabled.
   */
  class IA32MaydayHandler : public IMaydayHandler {
    public:
      /**
       * @brief Creates a new @ref IA32MaydayHandler.
       * @param context Pointer to the @ref KernelContext.
       */
      explicit IA32MaydayHandler(KernelContext* context);

      /**
       * @brief Handles a kernel mayday by logging diagnostics and halting.
       * @param message The mayday message describing the cause of the panic.
       * @note Does not return.
       *
       * Logs the mayday banner, current @ref Thread/@ref Process IDs, `ESP`,
       * a stack dump, and an `EBP`-chain call trace. Then halts the CPU with
       * interrupts disabled.
       */
      [[noreturn]]
      void Handle(const char* message) override;

    private:
      /**
       * @brief Pointer to the @ref KernelContext.
       */
      KernelContext* _context = nullptr;
  };
}
