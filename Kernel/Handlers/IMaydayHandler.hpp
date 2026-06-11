/**
 * @file Kernel/Handlers/IMaydayHandler.hpp
 * @brief Declares @ref @QKrnl::IMaydayHandler.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::Kernel::Handlers {
  /**
   * @brief Abstract interface for handling kernel mayday (panic) events.
   *
   * A mayday is the kernel's last-resort error path. Implementations
   * should log as much diagnostic information as possible (thread ID,
   * process ID, register state) and then halt the machine. The
   * @ref Handle method is marked `[[noreturn]]`; execution must never
   * continue after a mayday.
   */
  class IMaydayHandler {
    public:
      /**
       * @brief Destroys this @ref IMaydayHandler instance.
       */
      virtual ~IMaydayHandler() = default;

      /**
       * @brief Handles a kernel mayday with the specified message.
       * @param message The mayday message.
       */
      [[noreturn]] virtual void Handle(const char* message) = 0;
  };
}
