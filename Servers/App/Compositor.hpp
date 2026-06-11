/**
 * @file Servers/App/Compositor.hpp
 * @brief Declares @ref @QAppSrv::Compositor.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

#include <Quantum/Theme.hpp>

namespace Quantum::Servers::App {
  /**
   * @brief Owns the compositing buffers, dirty-rectangle management, and the
   *        asynchronous render pipeline (including the render thread).
   *
   * All pixel-level operations (fills, glyph rendering, buffer blits) live
   * here.  The compositor reads the window list through an injected
   * @ref WindowManager pointer and flushes completed frames through an
   * injected @ref GraphicsClient pointer.
   */
  class Compositor {
    public:
      /**
       * @brief Whether XOR outlines are drawn around flush regions for
       *        compositor debugging.
       *
       * Currently togglable during runtime with the Escape key.  Also
       * (currently) does not work with S3 ViRGE.
       */
      bool DebugFlushRegion = false;

      /**
       * @brief The 32-bit ARGB color value for the desktop background.
       */
      static constexpr UInt32 BackgroundColor = Theme::DesktopBackground;

      /**
       * @brief Maximum number of independent damage rectangles tracked before
       *        merging the two closest rects to make room.
       */
      static constexpr Size MaxDamageRects = 4;

      /**
       * @brief Creates a new compositor instance.
       */
      explicit Compositor();

      /**
       * @brief Destroys the compositor instance.
       */
      virtual ~Compositor() = default;

      /**
       * @brief Binds external dependencies and display parameters after
       *        construction.
       * @param graphics          IPC bridge for flushing to the graphics
       *                          server.
       * @param windowManager     Window list provider for compositing.
       * @param stateLock         Shared mutex protecting server state.
       * @param screenWidth       Logical screen width in pixels.
       * @param screenHeight      Logical screen height in pixels.
       * @param bufferBpp         Bytes per pixel (2 or 4).
       * @param bufferWidth       Row stride of the compositing buffer in
       *                          pixels.
       * @param compositeBuffer   Local shadow compositing buffer.
       * @param sharedBuffer      Graphics server's shared back buffer.
       * @param directFramebuffer Whether the back buffer is directly mapped
       *                          to VRAM.
       * @param hasHardwareCursor Whether the active driver provides a
       *                          hardware cursor.
       */
      void Init(
        GraphicsClient* graphics,
        WindowManager* windowManager,
        Sync::Mutex* stateLock,
        KernelClient* kernel,
        UInt16 screenWidth,
        UInt16 screenHeight,
        UInt8 bufferBpp,
        UInt16 bufferWidth,
        void* compositeBuffer,
        void* sharedBuffer,
        bool directFramebuffer,
        bool hasHardwareCursor
      );

      /**
       * @brief Fills a rectangle in the local compositing buffer with a solid
       *        ARGB32 color, clamped to screen bounds.
       * @param rect  The rectangle to fill.
       * @param color The 32-bit ARGB color value.
       */
      void BufferFillRectangle(Rectangle rect, UInt32 color);

      /**
       * @brief Initializes a content buffer with a solid ARGB32 color,
       *        converting to the compositor's native pixel format.
       * @param buffer     Pointer to the pixel buffer.
       * @param pixelCount Total number of pixels.
       * @param argbColor  The ARGB32 fill color.
       */
      void InitBuffer(void* buffer, UInt32 pixelCount, UInt32 argbColor);

      /**
       * @brief Renders a single glyph from the given bitmap font into the
       *        local compositing buffer.
       * @param x    Physical pixel x-coordinate.
       * @param y    Physical pixel y-coordinate.
       * @param ch   Character code.
       * @param fore 32-bit ARGB foreground color.
       * @param font The bitmap font to render from.
       * @param bold Whether to apply a bold effect.
       */
      void BufferRenderGlyph(
        Int16 x, Int16 y, UInt8 ch, UInt32 fore,
        const Fonts::BitmapFont& font, bool bold
      );

      /**
       * @brief Copies a rectangular region within the compositing buffer,
       *        handling overlapping source and destination correctly via
       *        direction-aware row ordering.
       * @param srcX Source x-coordinate (logical pixels).
       * @param srcY Source y-coordinate (logical pixels).
       * @param dstX Destination x-coordinate (logical pixels).
       * @param dstY Destination y-coordinate (logical pixels).
       * @param w    Width of the region to copy.
       * @param h    Height of the region to copy.
       */
      void BufferBlit(
        Int16 srcX, Int16 srcY,
        Int16 dstX, Int16 dstY,
        UInt16 w, UInt16 h
      );

      /**
       * @brief Creates a @ref DrawContext configured for the active rendering
       *        path (compositor buffer or legacy IPC).
       * @return The configured draw context.
       */
      DrawContext CreateDrawContext();

      /**
       * @brief Composites a single rectangular strip: fills with background
       *        color, then draws all windows clipped to the strip bounds.
       * @param strip The strip rectangle to composite.
       * @param ctx   The draw context for rendering.
       */
      void CompositeStrip(Rectangle strip, DrawContext& ctx);

      /**
       * @brief Expands the dirty region to include the given rectangle,
       *        clamped to screen bounds.
       * @param rect The rectangle to mark as damaged.
       */
      void Damage(Rectangle rect);

      /**
       * @brief Resets the dirty rectangle list.
       */
      void ClearDamage();

      /**
       * @brief Signals the render thread to perform a new render cycle.
       */
      void SignalRender();

      /**
       * @brief Spins until the render thread's compositing flag is cleared,
       *        yielding while the lock is temporarily released.
       */
      void WaitForCompositing();

      /**
       * @brief Static entry point for the render thread.
       * @param arg Opaque pointer (cast to @c Compositor*).
       */
      static void RenderThreadEntry(UInt32 arg);

      /**
       * @brief Main loop of the render thread.  Waits for a signal,
       *        snapshots damage, composites each dirty rect, and flushes.
       */
      void RenderLoop();

      /**
       * @brief Synchronous single-shot render used during initialisation.
       *        Composites and flushes all pending damage, then clears it.
       */
      void RenderImmediate();

      /**
       * @brief Returns the local compositing buffer pointer.
       * @return Pointer to the compositing buffer, or @c nullptr.
       */
      void* GetCompositeBuffer() const;

      /**
       * @brief Returns the shared graphics server back buffer pointer.
       * @return Pointer to the shared buffer, or @c nullptr.
       */
      void* GetSharedBuffer() const;

      /**
       * @brief Returns the bytes-per-pixel of the compositing buffer.
       * @return 2 for RGB565, 4 for ARGB32.
       */
      UInt8 GetBufferBpp() const;

      /**
       * @brief Returns the row width of the compositing buffer in pixels.
       * @return Buffer stride in pixels.
       */
      UInt16 GetBufferWidth() const;

      /**
       * @brief Returns whether the back buffer is directly mapped to VRAM.
       * @return @c true if direct framebuffer mode is active.
       */
      bool IsDirectFramebuffer() const;

      /**
       * @brief Returns a reference to the render signal futex, allowing
       *        external code (e.g. @ref GraphicsClient::CopyToShared) to
       *        check for interruption.
       * @return Mutable reference to the render signal word.
       */
      volatile UInt32& GetRenderSignal();

      /**
       * @brief Sets the overlay manager for rendering overlays above
       *        windows.
       * @param overlayManager Pointer to the overlay manager.
       */
      void SetOverlayManager(OverlayManager* overlayManager) {
        _overlayManager = overlayManager;
      }

      /**
       * @brief Sets the dock for bottom-of-screen rendering.
       */
      void SetDock(Dock::Dock* dock) { _dock = dock; }


    private:
      /**
       * @brief Pointer to the local compositing buffer.  Format is native
       *        display format (RGB565 for 16bpp, ARGB32 for 32bpp).
       */
      void* _compositeBuffer = nullptr;

      /**
       * @brief Pointer to the graphics server's shared back buffer.  When
       *        double-buffering is active, dirty regions are copied here from
       *        @ref _compositeBuffer just before the flush IPC.
       */
      void* _sharedBuffer = nullptr;

      /**
       * @brief Bytes per pixel of the compositing buffer (2 for RGB565,
       *        4 for ARGB32).
       */
      UInt8 _bufferBpp = 4;

      /**
       * @brief Row width of the compositing buffer in pixels (= screen width
       *        in physical pixels).
       */
      UInt16 _bufferWidth = 0;

      /**
       * @brief Whether the graphics server's back buffer is directly mapped
       *        into the application server's address space.
       */
      bool _directFramebuffer = false;

      /**
       * @brief Array of pending dirty rectangles in logical pixels.
       */
      Rectangle _dirtyRects[MaxDamageRects];

      /**
       * @brief Number of active entries in @ref _dirtyRects.
       */
      Size _dirtyRectCount = 0;

      /**
       * @brief Signal used to notify the render thread of new damage to
       *        composite.
       */
      volatile UInt32 _renderSignal = 0;

      /**
       * @brief Flag indicating whether the render thread is currently
       *        compositing.  The main thread waits for this to be false
       *        before allowing state changes that would affect the current
       *        render cycle.
       */
      volatile UInt32 _compositing = 0;

      /**
       * @brief IPC bridge for flushing completed frames to the graphics
       *        server.
       */
      GraphicsClient* _graphics = nullptr;

      /**
       * @brief Window list provider for compositing.
       */
      WindowManager* _windowManager = nullptr;

      /**
       * @brief Overlay manager for rendering system overlays above windows.
       */
      OverlayManager* _overlayManager = nullptr;

      /**
       * @brief The dock taskbar rendered at the bottom of the screen.
       */
      Dock::Dock* _dock = nullptr;

      /**
       * @brief Reference to the kernel client for thread operations.
       */
      KernelClient* _kernel = nullptr;

      /**
       * @brief Shared mutex protecting server state during request handling.
       */
      Sync::Mutex* _stateLock = nullptr;

      /**
       * @brief Logical screen width in pixels.
       */
      UInt16 _screenWidth = 0;

      /**
       * @brief Logical screen height in pixels.
       */
      UInt16 _screenHeight = 0;

      /**
       * @brief Whether the active graphics driver provides a hardware
       *        cursor.  When @c true, @c ShowCursor wrapping around drawing
       *        operations is skipped.
       */
      bool _hasHardwareCursor = false;

      /**
       * @brief Converts an ARGB32 color to RGB565.
       * @param argb The 32-bit ARGB color value.
       * @return The packed 16-bit RGB565 value.
       */
      UInt16 ToRGB565(UInt32 argb);

  };
}
