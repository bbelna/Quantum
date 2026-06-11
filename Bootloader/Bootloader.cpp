/**
 * @file Bootloader/Bootloader.cpp
 * @brief Implements @ref @QBtldr::Bootloader.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Version.hpp>

#include "Bootloader.hpp"
#include "BootloaderContext.hpp"
#include "BootloaderOptions.hpp"
#include "IBootInfo.hpp"
#include "IKernelLauncher.hpp"
#include "Shell.hpp"
#include "Strings.hpp"

#define S 1000
#define SKIP_BOOTLOADER_SHELL

namespace Quantum::Bootloader {
  [[noreturn]] void Bootloader::Run(
    IBootInfo* info,
    BootloaderContext* context
  ) {
    _context = context;
    _info = info;

    _context->PlatformInitializer->Initialize({ _context, _info });
    _context->Graphics->SetTextCursor(
      0,
      _context->Graphics->GetTextRows() - 1
    );
    _context->Graphics->SetTextColor(0x07, 0x00);
    _context->Graphics->Print(
      "Quantum " QUANTUMOS_RELEASE " " PLATFORM "/" TARGET_ARCH " " __DATE__
      " " __TIME__
    );
    _context->Graphics->SetTextColor(0x0F, 0x00);
    _context->Graphics->SetTextCursor(0, 0);

    if (!_context->FileLoader) {
      _printError(STRINGS_ERROR_NO_FILE_LOADER);

      _context->CPU->HaltForever();
    }

    static char kernelPath[] = "System/Kernel.qbn";
    static char initialImagePath[] = "System/Startup.qbd";
    static char initialProcessName[] = "StartupServer.qbn";

    static BootloaderOptions options = {
      kernelPath,
      initialImagePath,
      initialProcessName
    };

    _options = &options;
    _context->InitialProcessName = _options->InitialProcessName;

    #if defined(VERBOSE)
    _context->Graphics->Print(STRINGS_WELCOME);
    #endif

    #if not defined(SKIP_BOOTLOADER_SHELL)
    UInt32 start = _context->Timer->GetMS();
    UInt32 lastDot = start;

    // 3 second delay with periodic dot updates, or until a keypress triggers
    // the shell
    while (_context->Timer->GetMS() - start < 3 * S) {
      UInt32 now = _context->Timer->GetMS();

      if (now - lastDot >= S) {
        _context->Graphics->Print(".");

        lastDot = now;
      }

      if (_context->Keyboard->IsKeyReady()) {
        _context->Keyboard->ReadKey(); // consume the trigger key

        _resetCurrentTextRow();

        _context->Graphics->Print(STRINGS_ENTERING_SHELL);

        _runShell();

        break;
      }
    }
    #endif

    if (!_ranShell) _resetCurrentTextRow();

    // retry loop: on any load failure, drop back to the shell so the user
    // can fix paths, then try again; _runShell() returns normally so there
    // is no stack growth relative to a single-attempt path
    while (true) {
      if (!_loadKernel())  {
        _runShell();

        continue;
      }

      if (!_loadInitialImage()) {
        _runShell();

        continue;
      }

      break;
    }

    _context->PlatformInitializer->PrepareKernelHandoff({ _context, _info });
    _context->KernelLauncher->Launch(_info, _context);

    __builtin_unreachable();
  }

  void Bootloader::_runShell() {
    BootloaderShell shell(_context);

    shell.Run(_options);

    _ranShell = true;
  }

  bool Bootloader::_loadKernel() {
    _resetCurrentTextRow();

    #if defined(VERBOSE)
    _context->Graphics->Print(STRINGS_LOADING_KERNEL);
    #endif

    bool success = _context->FileLoader->Load(
      _options->KernelPath,
      _context->KernelPhysicalAddress,
      nullptr
    );

    if (!success) {
      _printError(
        STRINGS_ERROR_PREFIX STRINGS_LOAD_ERROR_PT1,
        _options->KernelPath,
        STRINGS_LOAD_ERROR_PT2
      );
    }

    return success;
  }

  bool Bootloader::_loadInitialImage() {
    _resetCurrentTextRow();

    #if defined(VERBOSE)
    _context->Graphics->Print(STRINGS_LOADING_INITIAL_IMAGE);
    #endif

    bool success = _context->FileLoader->Load(
      _options->InitialImagePath,
      _context->InitialImagePhysicalAddress,
      &_context->InitialImageSizeInBytes
    );

    if (!success) {
      _printError(
        STRINGS_ERROR_PREFIX STRINGS_LOAD_ERROR_PT1,
        _options->InitialImagePath,
        STRINGS_LOAD_ERROR_PT2
      );
    }

    return success;
  }

  void Bootloader::_resetCurrentTextRow() {
    UInt8 currentRow = _context->Graphics->GetTextCursorRow();

    _context->Graphics->ClearRow(currentRow);
    _context->Graphics->SetTextCursor(0, currentRow);
  }
}
