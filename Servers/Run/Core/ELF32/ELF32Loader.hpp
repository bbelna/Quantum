/**
 * @file Servers/Run/Core/ELF32/ELF32Loader.hpp
 * @brief Declares @ref @QRunSrv::ELF32Loader.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <RunServerTypes.hpp>

namespace Quantum::Servers::Run::Core::ELF32 {
  /**
   * @brief User-mode ELF32 loader.
   *
   * Parses an ELF image, lays out its loadable segments into a flat buffer,
   * and spawns a new @ref Process.
   */
  class ELF32Loader {
    public:
      /**
       * @brief
       *   Loads an ELF32 image and spawns a new @ref Process.
       * @param name
       *   Human-readable name for the new @ref Process.
       * @param image
       *   @ref UInt8 pointer to the raw ELF image data.
       * @param size
       *   @ref Size of the image in bytes.
       * @param argumentCount
       *   Number of command-line arguments.
       * @param argumentData
       *   Packed null-terminated argument strings, or `nullptr` if
       *   @p argumentCount is zero.
       * @param argumentDataSize
       *   Total @ref Size in bytes of @p argumentData.
       * @param streamCount
       *   Number of inherited stream buffer IDs (`0`-`3`).
       * @param streamBufferIDs
       *   Array of @ref SharedBufferID for inherited streams, or `nullptr` if
       *   none.
       * @return
       *   The new @ref Process @ref ProcessID, or @ref InvalidProcessID on
       *   failure.
       */
      static ProcessID Load(
        const char* name,
        const UInt8* image,
        Size size,
        Size argumentCount = 0,
        const char* argumentData = nullptr,
        Size argumentDataSize = 0,
        UInt8 streamCount = 0,
        const SharedBufferID* streamBufferIDs = nullptr
      );
  };
}
