/**
 * @file Bootloader/IFileLoader.hpp
 * @brief Declares @ref @QBtldr::IFileLoader.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <BootloaderTypes.hpp>
#include "ISpinner.hpp"

namespace Quantum::Bootloader {
  /**
   * @brief Abstract interface for loading files in the bootloader
   *        environment.
   */
  class IFileLoader {
    public:
      /**
       * @brief Callback invoked during file loading progress.
       *
       * Called after each successfully read unit (e.g. sector) when set.
       */
      using ProgressCallback = void (*)();

      /**
       * @brief Destroys this @ref IFileLoader instance.
       */
      virtual ~IFileLoader() = default;

      /**
       * @brief Loads a file from the specified path into memory.
       * @param path The file path to load.
       * @param address Physical address to load the file at.
        * @param outLoadedSize Receives loaded file size in bytes on success;
        *                      optional.
       * @return `true` on success; `false` on failure.
       */
      virtual bool Load(
        const char* path,
        UInt32 address,
        UInt32* outLoadedSize = nullptr
      ) = 0;

      /**
       * @brief Sets this file loader's @ref ISpinner.
       * @param spinner Pointer to the spinner instance, or `nullptr` to
                        clear.
       */
      virtual void SetSpinner(ISpinner* spinner) = 0;
  };
}
