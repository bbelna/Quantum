/**
 * @file Include/Quantum/Clients/DisplayClient.hpp
 * @brief Declares @ref @QClients::DisplayClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Geometry2D.hpp>
#include <Quantum/HAL.hpp>
#include <Quantum/Types.hpp>

#include "KernelClient.hpp"

namespace Quantum::Clients {
  namespace {
    using namespace Quantum::Geometry2D;
  }

  /**
   * @brief Client-side interface for text-mode display operations.
   *
   * Talks directly to the kernel graphics driver via
   * @ref KernelClient::InvokeDriver. Used by text-mode programs that acquire
   * the display and render text directly.
   *
   * @code
   *   DisplayClient display;
   *
   *   if (display.Initialize() && display.AcquireDisplay()) {
   *     display.WriteText("Hello from text mode!\n");
   *     display.ReleaseDisplay();
   *   }
   * @endcode
   */
  class DisplayClient {
    public:
      /**
       * @brief Creates a new @ref DisplayClient.
       */
      DisplayClient() = default;

      /**
       * @brief Discovers the graphics device and caches its device ID.
       * @return `true` if a graphics device was found.
       */
      bool Initialize();

      /**
       * @brief Acquires exclusive display ownership for the calling process.
       * @return `true` if ownership was granted.
       */
      bool AcquireDisplay();

      /**
       * @brief Releases display ownership, reverting to the default state.
       */
      void ReleaseDisplay();

      /**
       * @brief Returns whether the display is currently in compositing mode.
       * @return `true` if a process holds the display.
       */
      bool IsCompositing();

      /**
       * @brief Writes a null-terminated string at the current text cursor.
       * @param text The text to write.
       */
      void WriteText(const char* text);

      /**
       * @brief Sets the text foreground color.
       * @param color 32-bit ARGB color value.
       */
      void SetTextForegroundColor(UInt32 color);

      /**
       * @brief Gets the current text cursor position.
       * @return The cursor position (X = column, Y = row).
       */
      Point GetTextCursorPosition();

      /**
       * @brief Sets the text cursor position.
       * @param position The new position.
       */
      void SetTextCursorPosition(Point position);

      /**
       * @brief Queries the current display mode.
       * @param outWidth Receives the display width in pixels.
       * @param outHeight Receives the display height in pixels.
       * @param outBpp Receives the bits per pixel.
       * @return `true` on success.
       */
      bool GetModeInfo(UInt16* outWidth, UInt16* outHeight, UInt8* outBpp);

    private:
      KernelClient _kernel;

      /**
       * @brief Cached graphics device ID, or 0 if not initialized.
       */
      UInt32 _deviceID = 0;

      /**
       * @brief Whether the display has been acquired by this client.
       */
      bool _acquired = false;
  };
}
