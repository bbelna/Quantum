/**
 * @file Bootloader/Shell/BootloaderShell.cpp
 * @brief Implements @ref @QBtldr::Shell::BootloaderShell.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <BootloaderContext.hpp>
#include <BootloaderOptions.hpp>
#include <Strings.hpp>

#include "BootloaderShell.hpp"

namespace Quantum::Bootloader::Shell {
  BootloaderShell::BootloaderShell(BootloaderContext* context)
    : _context(context) {}

  BootloaderOptions* BootloaderShell::Run(BootloaderOptions* options) {
    _context->Graphics->Print(STRINGS_SHELL_HELP_PROMPT);

    while (true) {
      char command[MaxCommandLength + 1] = {};
      UInt32 length = 0;

      _context->Graphics->Print(STRINGS_SHELL_PROMPT);
      _context->Graphics->ShowTextCursor();

      while (true) {
        UInt8 key = _context->Keyboard->ReadKey();

        if (key == '\r') {
          _context->Graphics->Print("\n");

          break;
        }

        if (key == '\b') {
          if (length > 0) {
            length--;

            _context->Graphics->Print("\b \b");
          }

          continue;
        }

        if (key < 0x20 || key > 0x7E) continue;

        if (length < MaxCommandLength) {
          command[length++] = _toLower(static_cast<char>(key));

          char echo[2] = {static_cast<char>(key), '\0'};

          _context->Graphics->Print(echo);
        }
      }

      command[length] = '\0';

      _context->Graphics->HideTextCursor();

      if (_isBootCommand(command, length)) {
        return options;
      }

      PrintUnknownCommand(command);
    }
  }

  char BootloaderShell::_toLower(char c) {
    if (c >= 'A' && c <= 'Z') return static_cast<char>(c - 'A' + 'a');

    return c;
  }

  bool BootloaderShell::_isBootCommand(const char* command, UInt32 length) {
    return
      length == 4 &&
      command[0] == 'b' &&
      command[1] == 'o' &&
      command[2] == 'o' &&
      command[3] == 't';
  }

  void BootloaderShell::PrintUnknownCommand(const char* command) {
    _context->Graphics->Print(STRINGS_SHELL_UNKNOWN_COMMAND_PT1);
    _context->Graphics->Print(command);
    _context->Graphics->Print(STRINGS_SHELL_UNKNOWN_COMMAND_PT2);
  }
}
