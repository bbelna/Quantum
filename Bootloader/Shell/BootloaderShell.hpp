/**
 * @file Bootloader/Shell/BootloaderShell.hpp
 * @brief Declares @ref @QBtldr::Shell::BootloaderShell.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Shell/ShellTypes.hpp>

namespace Quantum::Bootloader::Shell {
  /**
   * @brief Interactive bootloader shell.
   *
   * Prompts the user for commands, applies any options entered, and returns
   * when the user types `boot`. The shell uses the @ref BootloaderContext
   * exclusively for all I/O so it remains architecture-agnostic.
   */
  class BootloaderShell {
    public:
      /**
       * @brief Maximum number of characters accepted in a single command.
       */
      static constexpr UInt32 MaxCommandLength = 15;

      /**
       * @brief Constructs a @ref BootloaderShell backed by the given context.
       * @param context Pointer to the active @ref BootloaderContext. Must
       *                remain valid for the lifetime of this object.
       */
      explicit BootloaderShell(BootloaderContext* context);

      /**
       * @brief Runs the shell until the user enters `boot`.
       * @param options Pointer to the @ref BootloaderOptions to inspect and
       *                modify based on user input.
       * @return The same @p options pointer, updated with any options the
       *         user has set during the session.
       *
       * Repeatedly prints a `>` prompt, reads a line of input, and dispatches
       * the command. Returns only when the user enters `boot`.
       */
      BootloaderOptions* Run(BootloaderOptions* options);

    private:
      /**
       * @brief The bootloader context providing I/O services.
       */
      BootloaderContext* _context;

      /**
       * @brief Converts an ASCII uppercase letter to lowercase.
       * @param c The input character.
       * @return The lowercase equivalent if @p c is `A`–`Z`; otherwise @p c.
       */
      static char _toLower(char c);

      /**
       * @brief Checks whether @p command equals `"boot"`.
       * @param command Null-terminated command string (already lowercased).
       * @param length  Length of the command, excluding the null terminator.
       * @return `true` if the command is exactly `"boot"`.
       */
      static bool _isBootCommand(const char* command, UInt32 length);

      /**
       * @brief Prints an unrecognized-command message.
       * @param command Null-terminated string containing the unrecognized
       *                command.
       */
      void PrintUnknownCommand(const char* command);
  };
}
