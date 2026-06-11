/**
 * @file Servers/App/Cursor/CursorManager.hpp
 * @brief Declares @ref CursorManager, the cursor state and bitmap manager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Cursors/CursorImage.hpp>

#include <AppServerTypes.hpp>

namespace Quantum::Servers::App::Cursor {
  /**
   * @brief Manages cursor position, clamping, and bitmap switching.
   *
   * Loads cursor bitmaps from a QCUR file on disk at initialization
   * time, falling back to the hardcoded arrow bitmap if the file is
   * not available. Supports switching between cursor types (arrow,
   * I-beam, crosshair, resize, etc.) via @ref SetCursor.
   */
  class CursorManager {
    public:
      /**
       * @brief Constructs a @ref CursorManager.
       * @param graphicsClient Reference to a @ref GraphicsClient.
       * @param screenWidth Logical screen width.
       * @param screenHeight Logical screen height.
       */
      explicit CursorManager(
        GraphicsClient& graphicsClient,
        UInt16 screenWidth,
        UInt16 screenHeight
      );

      /**
       * @brief Destroys the @ref CursorManager and frees the cursor file
       *        buffer.
       */
      virtual ~CursorManager();

      /**
       * @brief Centers the cursor, loads cursor bitmaps from disk,
       *        and registers the default arrow bitmap with the graphics
       *        server.
       */
      void Init();

      /**
       * @brief Applies a relative delta to the cursor position, clamped to
       *        screen bounds, and sends a move IPC.
       * @param dx Horizontal delta in logical pixels.
       * @param dy Vertical delta in logical pixels.
       * @return The new cursor position after clamping.
       */
      Point UpdatePosition(Int16 dx, Int16 dy);

      /**
       * @brief Sets the cursor position directly and sends a move IPC.
       * @param p The new cursor position (logical pixels).
       * @param suppressFlush If `true`, the graphics server will not flush
       *        the cursor region to the display.
       */
      void MoveTo(Point point, bool suppressFlush = false);

      /**
       * @brief Returns the current cursor position.
       * @return The cursor position in logical pixels.
       */
      Point GetPosition() const;

      /**
       * @brief Switches to the specified cursor type if it differs from
       *        the current cursor.
       * @param type The cursor type to activate.
       */
      void SetCursor(CursorType type);

      /**
       * @brief Compatibility wrapper: switches between the arrow and
       *        resize cursor bitmaps.
       * @param resize `true` to use the NWSE resize cursor, `false`
       *        for the arrow.
       */
      void SetResizeCursor(bool resize);

      /**
       * @brief Shows or hides the cursor via the graphics server.
       * @param visible `true` to show the cursor, `false` to hide it.
       */
      void Show(bool visible);

    private:
      /**
       * @brief Reference to the graphics bridge for IPC delegation.
       */
      GraphicsClient& _graphicsClient;

      /**
       * @brief Current cursor screen position in logical pixels.
       */
      Point _position;

      /**
       * @brief The currently active cursor type.
       */
      CursorType _currentCursor = CursorType::Arrow;

      /**
       * @brief Logical screen width for clamping.
       */
      UInt16 _screenWidth;

      /**
       * @brief Logical screen height for clamping.
       */
      UInt16 _screenHeight;

      /**
       * @brief Parsed cursor set loaded from the QCUR file.
       */
      CursorImage _cursorSet = {};

      /**
       * @brief Virtual address of the QCUR file buffer, or `0` if
       *        loading failed.
       */
      UIntPtr _cursorFileBuffer = 0;

      /**
       * @brief Loads the system cursor set from disk.
       */
      void _loadCursors();

      /**
       * @brief Bakes a drop shadow into a cursor bitmap and sends the
       *        result to the graphics server. Takes a raw 16x16 bitmap
       *        and produces a 24x24 bitmap with shadow padding.
       * @param pixels Raw cursor bitmap (16x16 ARGB32, row-major).
       */
      void _sendBitmapWithShadow(const UInt32* pixels);

      /**
       * @brief Heap-allocated scratch buffer for the shadow-expanded
       *        cursor bitmap (24x24). Allocated in constructor, freed
       *        in destructor.
       */
      UInt32* _expandedBitmap = nullptr;
  };
}
