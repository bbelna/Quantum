/**
 * @file Include/Quantum/Clients/GraphicsClient.hpp
 * @brief Declares @ref @QClients::GraphicsClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Geometry2D.hpp>
#include <Quantum/Kernel/IPC/IPCTypes.hpp>

#include "KernelClient.hpp"

namespace Quantum::Clients {
  /**
   * @brief Shared state that the graphics client needs read access to from
   *        the compositor and server.
   *
   * Pointers are set after construction via
   * @ref GraphicsClient::SetSharedState once the compositor has allocated
   * its buffers and queried display geometry.
   */
  struct GraphicsClientShared {
    /**
     * @brief Logical screen width in pixels.
     */
    UInt16* ScreenWidth = nullptr;

    /**
     * @brief Logical screen height in pixels.
     */
    UInt16* ScreenHeight = nullptr;

    /**
     * @brief Pointer to the local compositing buffer.
     */
    void** CompositeBuffer = nullptr;

    /**
     * @brief Pointer to the graphics server's shared back buffer.
     */
    void** SharedBuffer = nullptr;

    /**
     * @brief Bytes per pixel of the compositing buffer (2 or 4).
     */
    UInt8* BufferBpp = nullptr;

    /**
     * @brief Row width of the compositing buffer in pixels.
     */
    UInt16* BufferWidth = nullptr;

    /**
     * @brief Whether the graphics server's back buffer is directly mapped.
     */
    bool* DirectFramebuffer = nullptr;

    /**
     * @brief Render signal flag checked during buffer copies to detect
     *        interruption by a new render cycle.
     */
    volatile UInt32* RenderSignal = nullptr;

    /**
     * @brief Pointer to the debug flush region toggle on the owning server.
     */
    bool* DebugFlushRegion = nullptr;
  };

  /**
   * @brief Client-side interface to the graphics server.
   *
   * Wraps all graphics-server IPC calls (fill, XOR, blit, cursor, flush,
   * draw text) behind a cached send handle for high-frequency use.
   * Coordinates are accepted in logical pixels and scaled by the display
   * scale factor before transmission.
   *
   * @code
   *   GraphicsClient graphics(gfxHandle, flushReplyHandle,
   *                           flushReplyPortID, scaleFactor);
   *   graphics.FillRectangle(0, 0, 800, 600, 0xFF336699);
   *   graphics.FlushBackBuffer(0, 0, 800, 600);
   * @endcode
   */
  class GraphicsClient {
    public:
      /**
       * @brief The 32-bit ARGB color value for the outline drawn when
       *        dragging a window.
       */
      static constexpr UInt32 OutlineColor = 0xFFFFFFFF;

      /**
       * @brief The width of the raw cursor artwork in pixels.
       */
      static constexpr UInt8 CursorArtWidth = 16;

      /**
       * @brief The height of the raw cursor artwork in pixels.
       */
      static constexpr UInt8 CursorArtHeight = 16;

      /**
       * @brief The width of the cursor bitmap sent to the graphics server,
       *        including shadow padding.
       */
      static constexpr UInt8 CursorWidth = 24;

      /**
       * @brief The height of the cursor bitmap sent to the graphics server,
       *        including shadow padding.
       */
      static constexpr UInt8 CursorHeight = 24;

      /**
       * @brief The 32-bit ARGB transparent color for cursor bitmaps.
       */
      static constexpr UInt32 CursorTransparent = 0x00000000;

      /**
       * @brief Constructs a graphics client with the given IPC handles.
       * @param gfxHandle IPC send handle for the graphics server port.
       * @param flushReplyHandle IPC receive handle for flush
       *        acknowledgements.
       * @param flushReplyPortID Port ID for receiving flush
       *        acknowledgements.
       * @param scaleFactor Display scale factor (logical to physical).
       */
      explicit GraphicsClient(
        IPCPortResourceID gfxHandle,
        IPCPortResourceID flushReplyHandle,
        IPCPortID flushReplyPortID,
        UInt8 scaleFactor
      );

      /**
       * @brief Destroys the graphics client instance.
       */
      virtual ~GraphicsClient() = default;

      /**
       * @brief Binds the shared compositor state that is owned by the
       *        server.
       * @param shared The shared state pointers.
       */
      void SetSharedState(const GraphicsClientShared& shared);

      /**
       * @brief Sends a fill rectangle request to the graphics server.
       * @param x x-coordinate of the rectangle (logical pixels).
       * @param y y-coordinate of the rectangle (logical pixels).
       * @param w Width of the rectangle (logical pixels).
       * @param h Height of the rectangle (logical pixels).
       * @param color The 32-bit ARGB color value.
       */
      void FillRectangle(
        UInt16 x,
        UInt16 y,
        UInt16 w,
        UInt16 h,
        UInt32 color
      );

      /**
       * @brief Sends an XOR rectangle request to the graphics server.
       * @param x x-coordinate of the rectangle (logical pixels).
       * @param y y-coordinate of the rectangle (logical pixels).
       * @param w Width of the rectangle (logical pixels).
       * @param h Height of the rectangle (logical pixels).
       * @param color The 32-bit ARGB color value to XOR with.
       */
      void XORRectangle(
        UInt16 x,
        UInt16 y,
        UInt16 w,
        UInt16 h,
        UInt32 color
      );

      /**
       * @brief Sends a screen blit request to the graphics server.
       * @param srcX Source x-coordinate (logical pixels).
       * @param srcY Source y-coordinate (logical pixels).
       * @param dstX Destination x-coordinate (logical pixels).
       * @param dstY Destination y-coordinate (logical pixels).
       * @param w Width of the region to copy.
       * @param h Height of the region to copy.
       */
      void ScreenBlit(
        UInt16 srcX,
        UInt16 srcY,
        UInt16 dstX,
        UInt16 dstY,
        UInt16 w,
        UInt16 h
      );

      /**
       * @brief Sends a screen blit request and blocks until the graphics
       *        server acknowledges the GPU blit has completed.
       * @param srcX Source x-coordinate (logical pixels).
       * @param srcY Source y-coordinate (logical pixels).
       * @param dstX Destination x-coordinate (logical pixels).
       * @param dstY Destination y-coordinate (logical pixels).
       * @param w Width of the region to copy.
       * @param h Height of the region to copy.
       */
      void ScreenBlitSync(
        UInt16 srcX,
        UInt16 srcY,
        UInt16 dstX,
        UInt16 dstY,
        UInt16 w,
        UInt16 h
      );

      /**
       * @brief Sends a cursor move request to the graphics server.
       * @param p The new cursor position (logical pixels).
       * @param suppressFlush If `true`, the graphics server will not flush
       *        the cursor region to the display immediately.
       */
      void MoveCursor(Geometry2D::Point p, bool suppressFlush = false);

      /**
       * @brief Sends a show/hide cursor request to the graphics server.
       * @param visible `true` to show the cursor, `false` to hide it.
       */
      void ShowCursor(bool visible);

      /**
       * @brief Sends a `BeginBatch` request to the graphics server.
       */
      void BeginBatch();

      /**
       * @brief Sends an `EndBatch` request to the graphics server.
       */
      void EndBatch();

      /**
       * @brief Sends a `FlushBackBuffer` request to the graphics server,
       *        triggering a flush of the specified dirty region to the
       *        display.
       * @param x x-coordinate of the dirty region (logical pixels).
       * @param y y-coordinate of the dirty region (logical pixels).
       * @param w Width of the dirty region (logical pixels).
       * @param h Height of the dirty region (logical pixels).
       * @param sync If `true` (default), uses deferred mode and tracks
       *        pending flush count. If `false`, sends fire-and-forget.
       */
      void FlushBackBuffer(
        UInt16 x,
        UInt16 y,
        UInt16 w,
        UInt16 h,
        bool sync = true
      );

      /**
       * @brief Blocks until all deferred FlushBackBuffer acks have been
       *        received. Must be called before any compositor buffer write.
       */
      void WaitForPendingFlush();

      /**
       * @brief Sends a `DrawText` request to the graphics server.
       * @param x x-coordinate of the text (logical pixels).
       * @param y y-coordinate of the text (logical pixels).
       * @param title The null-terminated string to draw.
       * @param maxLen The maximum number of characters to draw.
       * @param color The 32-bit ARGB text color.
       * @param fontWeight Font weight selector.
       * @param glyphWidth Width of a single glyph in pixels (for
       *        off-screen clipping).
       * @param glyphHeight Height of a single glyph in pixels (for
       *        off-screen clipping).
       */
      void DrawText(
        Int16 x,
        Int16 y,
        const char* title,
        Size maxLen,
        UInt32 color,
        UInt16 fontWeight,
        UInt8 glyphWidth,
        UInt8 glyphHeight
      );

      /**
       * @brief Sends a `SetCursorBitmap` request to the graphics server
       *        using the fixed 16x16 cursor format.
       * @param pixels Flat ARGB pixel array (CursorWidth * CursorHeight).
       */
      void SetCursorBitmap(const UInt32* pixels);

      /**
       * @brief Clips a signed rectangle to screen bounds and sends a fill
       *        request using the cached graphics handle.
       * @param rect The rectangle to fill (may be partially off-screen).
       * @param color The 32-bit ARGB color value.
       */
      void ClippedFillRectangle(
        Geometry2D::Rectangle rect,
        UInt32 color
      );

      /**
       * @brief Draws an XOR outline rectangle at the specified position
       *        and size, used for visual feedback when dragging windows.
       * @param x x-coordinate of the rectangle (logical pixels).
       * @param y y-coordinate of the rectangle (logical pixels).
       * @param w Width of the rectangle (logical pixels).
       * @param h Height of the rectangle (logical pixels).
       */
      void DrawOutline(Int16 x, Int16 y, UInt16 w, UInt16 h);

      /**
       * @brief Copies a rectangular region from the shadow compositor
       *        buffer to the shared graphics server buffer.
       * @param x x-coordinate of the region (logical pixels).
       * @param y y-coordinate of the region (logical pixels).
       * @param w Width of the region (logical pixels).
       * @param h Height of the region (logical pixels).
       * @return `true` if completed, `false` if interrupted by a new
       *         render signal.
       */
      bool CopyToShared(UInt16 x, UInt16 y, UInt16 w, UInt16 h);

    private:
      /**
       * @brief Kernel client for IPC and memory operations.
       */
      KernelClient _kernel;

      /**
       * @brief The IPC send handle for communicating with the graphics
       *        server.
       */
      IPCPortResourceID _gfxHandle;

      /**
       * @brief Receive handle for the `FlushBackBuffer` acknowledgement
       *        port.
       */
      IPCPortResourceID _gfxFlushReplyHandle;

      /**
       * @brief Auto-assigned port ID for receiving flush acknowledgements.
       */
      IPCPortID _gfxFlushReplyPortID;

      /**
       * @brief Whether a deferred FlushBackBuffer ack is pending.
       */
      bool _flushPending = false;

      /**
       * @brief Number of deferred flush acks expected on the next drain.
       */
      UInt8 _pendingFlushCount = 0;

      /**
       * @brief Display scale factor (logical to physical pixel multiplier).
       */
      UInt8 _scaleFactor;

      /**
       * @brief Shared compositor state pointers, bound via
       *        @ref SetSharedState.
       */
      GraphicsClientShared _shared;
  };
}
