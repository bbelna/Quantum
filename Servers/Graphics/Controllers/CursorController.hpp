/**
 * @file Servers/Graphics/Controllers/CursorController.hpp
 * @brief Declares @ref @QGfxSrv::CursorController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <GraphicsServerTypes.hpp>

#include "../Core/Display/Display.hpp"
#include "../Core/Cursor/Cursor.hpp"

namespace Quantum::Servers::Graphics {
  /**
   * @brief Handles cursor operations: SetCursorBitmap, MoveCursor,
   *        and ShowCursor.
   */
  class CursorController : public RequestController {
    public:
      CursorController(
        KernelClient& kernel,
        ServerLog& log,
        Display& display,
        Cursor& cursor
      );

    private:
      Display& _display;
      Cursor& _cursor;

      void _handleSetCursorBitmap(
        const GraphicsSetCursorBitmapRequest& request,
        const IPCMessage* message
      );

      void _handleMoveCursor(
        const GraphicsMoveCursorRequest& request
      );

      void _handleShowCursor(
        const GraphicsShowCursorRequest& request
      );
  };
}
