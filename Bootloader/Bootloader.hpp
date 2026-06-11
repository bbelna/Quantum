/**
 * @file Bootloader/Bootloader.hpp
 * @brief Declares @ref @QBtldr::Bootloader.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <BootloaderTypes.hpp>
#include "BootloaderContext.hpp"

namespace Quantum::Bootloader {
  struct IBootInfo;

  /**
   * @brief The main bootloader class.
   *
   * Implements the arch-agnostic boot flow: platform early-initialization,
   * countdown shell, kernel/startup loading, platform handoff, and kernel
   * launch. All platform and architecture specifics are injected through
   * @ref BootloaderContext.
   */
  class Bootloader {
    public:
      /**
       * @brief Runs the bootloader to completion.
       * @param info Pointer to the platform boot-info block, already cast from
       *             the raw physical address by the platform entry point before
       *             any other construction.
       * @param context Pointer to the fully-wired @ref BootloaderContext.
       *
       * Does not return; ends with @ref IKernelLauncher::Launch.
       */
      [[noreturn]] void Run(IBootInfo* info, BootloaderContext* context);

    private:
      /**
       * @brief Platform boot-info block set by @ref Run.
       */
      IBootInfo* _info = nullptr;

      /**
       * @brief Fully-wired bootloader context set by @ref Run.
       */
      BootloaderContext* _context = nullptr;

      /**
       * @brief Active bootloader options (file paths, etc.).
       */
      BootloaderOptions* _options = nullptr;

      /**
       * @brief Whether the interactive shell has been entered at least once.
       */
      bool _ranShell = false;

      /**
       * @brief Enters the interactive boot shell.
       *
       * Sets @ref _ranShell to `true` on return.
       */
      void _runShell();

      /**
       * @brief Loads the kernel image from disk.
       * @return `true` if the kernel was loaded successfully.
       */
      bool _loadKernel();

      /**
       * @brief Loads the initial image from disk.
       * @return `true` if the initial image was loaded successfully.
       */
      bool _loadInitialImage();

      /**
       * @brief Clears the current text row and resets the cursor to column `0`.
       */
      void _resetCurrentTextRow();

      /**
       * @brief Prints one or more string fragments as a single error line.
       * @tparam Args Pack of `const char*` arguments.
       * @param args The string fragments to print in order.
       *
       * Clears the current row, switches to bright-red text, prints each
       * fragment, then appends a newline.
       */
      template <typename... Args>
      void _printError(Args... args) {
        _resetCurrentTextRow();

        _context->Graphics->SetTextColor(0x0C, 0x00);
        (_context->Graphics->Print(args), ...);
        _context->Graphics->Print("\n");
        _context->Graphics->SetTextColor(0x0F, 0x00);
      }
  };
}
