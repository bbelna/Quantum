/**
 * @file Include/Quantum/HAL/Graphics/GraphicsDriver.hpp
 * @brief Declares @ref @QHAL::Graphics::GraphicsDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Geometry2D.hpp>

#include "../IDriver.hpp"
#include "GraphicsDriverOperation.hpp"
#include "Payloads.hpp"

namespace Quantum::HAL::Graphics {
  /**
   * @brief Abstract base class for graphics device drivers.
   */
  class GraphicsDriver : public HAL::IDriver {
    public:
      /**
       * @brief Writes text to the screen at the current text cursor position.
       * @param text The text to write.
       *
       * The driver's text cursor will be updated to the position immediately
       * following the last character of the written text.
       */
      virtual void WriteText(const char* text) = 0;

      /**
       * @brief Sets the foreground color for subsequent text rendering.
       * @param color The 32-bit ARGB color value.
       *
       * The default implementation is a no-op. Drivers that support colored
       * text output override this to update their internal text foreground
       * color.
       */
      virtual void SetTextForegroundColor(UInt32 color) { (void)color; }

      /**
       * @brief Gets the current text cursor position.
       * @return The current text cursor position.
       */
      virtual Geometry2D::Point GetTextCursorPosition() = 0;

      /**
       * @brief Sets the text cursor position.
       * @param position The new text cursor position.
       */
      virtual void SetTextCursorPosition(Geometry2D::Point position) = 0;

      /**
       * @brief Switches the display to the specified video mode.
       * @param mode The video mode number (e.g., 0x12 for 640x480x16, 0x13
       *        for 320x200x256, or a VESA mode such as 0x101).
       */
      virtual void SetMode(UInt16 mode) = 0;

      /**
       * @brief Fills a rectangle with a solid color.
       * @param x The x-coordinate of the top-left corner.
       * @param y The y-coordinate of the top-left corner.
       * @param w The width of the rectangle in pixels.
       * @param h The height of the rectangle in pixels.
       * @param color The color value. In palette modes, the low byte is the
       *        palette index.
       */
      virtual void FillRectangle(
        UInt16 x,
        UInt16 y,
        UInt16 w,
        UInt16 h,
        UInt32 color
      ) = 0;

      /**
       * @brief Copies a pixel buffer to the framebuffer.
       * @param x The x-coordinate of the top-left corner.
       * @param y The y-coordinate of the top-left corner.
       * @param w The width in pixels.
       * @param h The height in pixels.
       * @param transparentColor Pixels matching this value are skipped.
       * @param pixels The pixel data (`w` * `h` 32-bit ARGB values).
       */
      virtual void BlitBuffer(
        UInt16 x,
        UInt16 y,
        UInt16 w,
        UInt16 h,
        UInt32 transparentColor,
        const UInt32* pixels
      ) = 0;

      /**
       * @brief XORs a rectangle with a color (used for drawing the drag
       *        outline).
       * @param x The x-coordinate of the top-left corner.
       * @param y The y-coordinate of the top-left corner.
       * @param w The width in pixels.
       * @param h The height in pixels.
       * @param color The color value to XOR with.
       */
      virtual void XORRectangle(
        UInt16 x,
        UInt16 y,
        UInt16 w,
        UInt16 h,
        UInt32 color
      ) = 0;

      /**
       * @brief Queries the current video mode information.
       * @param info Pointer to a `Payloads::ModeInfoPayload` struct that the
       *        driver will fill with the current mode's width, height, bits
       *        per pixel, and pitch.
       */
      virtual void GetModeInfo(Payloads::ModeInfoPayload* info) = 0;

      /**
       * @brief Enables or disables batch mode. In batch mode, drawing
       *        operations update the shadow buffer only, deferring the
       *        expensive framebuffer copy until `FlushRegion()` is called.
       * @param enabled `true` to enable batch mode, false to disable.
       */
      virtual void SetBatchMode(bool enabled) { (void)enabled; }

      /**
       * @brief Copies a rectangular region from the shadow buffer to the
       *        framebuffer. Used to flush deferred writes after batch mode.
       * @param x The x-coordinate of the region's top-left corner.
       * @param y The y-coordinate of the region's top-left corner.
       * @param w The width of the region in pixels.
       * @param h The height of the region in pixels.
       */
      virtual void FlushRegion(UInt16 x, UInt16 y, UInt16 w, UInt16 h) {
        (void)x; (void)y; (void)w; (void)h;
      }

      /**
       * @brief Moves a rectangular region of the screen to a new position
       *        using screen-to-screen copy. On hardware that supports it,
       *        this uses the GPU's BITBLT engine for zero-copy VRAM moves.
       * @param srcX Source x-coordinate.
       * @param srcY Source y-coordinate.
       * @param dstX Destination x-coordinate.
       * @param dstY Destination y-coordinate.
       * @param w Width of the region in pixels.
       * @param h Height of the region in pixels.
       */
      virtual void ScreenBlit(
        UInt16 srcX, UInt16 srcY,
        UInt16 dstX, UInt16 dstY,
        UInt16 w, UInt16 h
      ) {
        (void)srcX; (void)srcY; (void)dstX; (void)dstY; (void)w; (void)h;
      }

      /**
       * @brief Queries whether this driver supports a hardware cursor.
       * @param out Pointer to a `Payloads::HardwareCursorSupportPayload` to
       *        fill.
       */
      virtual void GetHardwareCursorSupport(
        Payloads::HardwareCursorSupportPayload* out
      ) {
        if (out) out->Supported = false;
      }

      /**
       * @brief Uploads an ARGB cursor bitmap to the hardware cursor.
       * @param w Width of the source bitmap in pixels.
       * @param h Height of the source bitmap in pixels.
       * @param transparent Color value treated as transparent.
       * @param pixels Pointer to `w * h` 32-bit ARGB pixel values.
       */
      virtual void SetHardwareCursorBitmap(
        UInt8 w,
        UInt8 h,
        UInt32 transparent,
        const UInt32* pixels
      ) {
        (void)w; (void)h; (void)transparent; (void)pixels;
      }

      /**
       * @brief Moves the hardware cursor to the specified screen position.
       * @param x New x-coordinate (negative values are clamped to 0).
       * @param y New y-coordinate (negative values are clamped to 0).
       */
      virtual void SetHardwareCursorPosition(Int16 x, Int16 y) {
        (void)x; (void)y;
      }

      /**
       * @brief Shows or hides the hardware cursor.
       * @param visible `true` to show, false to hide.
       */
      virtual void SetHardwareCursorVisible(bool visible) {
        (void)visible;
      }

      /**
       * @brief Writes a rectangular region of ARGB32 pixels to the shadow
       *        buffer, converting to the native pixel format. The source
       *        is a sub-region of a larger buffer with the given pitch.
       * @param x Destination x-coordinate.
       * @param y Destination y-coordinate.
       * @param w Width of the region in pixels.
       * @param h Height of the region in pixels.
       * @param srcPitch Row width of the source buffer in pixels.
       * @param pixels Pointer to the ARGB32 source buffer. The region
       *        starts at `pixels[y * srcPitch + x]`.
       */
      virtual void WritePixelRegion(
        UInt16 x,
        UInt16 y,
        UInt16 w,
        UInt16 h,
        UInt32 srcPitch,
        const UInt32* pixels
      ) {
        (void)x; (void)y; (void)w; (void)h;
        (void)srcPitch; (void)pixels;
      }

      /**
       * @brief Writes a rectangular region of native-format pixels to the
       *        shadow buffer and framebuffer without format conversion.
       *        The source data must already be in the display's native pixel
       *        format (e.g. RGB565 for 16bpp modes).
       * @param x Destination x-coordinate.
       * @param y Destination y-coordinate.
       * @param w Width of the region in pixels.
       * @param h Height of the region in pixels.
       * @param srcPitch Row width of the source buffer in pixels.
       * @param pixels
       *   Pointer to the native-format source buffer. The region starts at
       *   byte offset `y * srcPitch * bytesPerPixel + x * bytesPerPixel`.
       */
      virtual void WriteNativeRegion(
        UInt16 x,
        UInt16 y,
        UInt16 w,
        UInt16 h,
        UInt32 srcPitch,
        const void* pixels
      ) {
        (void)x; (void)y; (void)w; (void)h;
        (void)srcPitch; (void)pixels;
      }

      /**
       * @brief Queries whether this driver supports hardware-accelerated
       *        screen-to-screen BLT.
       * @param out Pointer to a `Payloads::FastScreenBlitSupportPayload` to
       *        fill.
       */
      virtual void GetFastScreenBlitSupport(
        Payloads::FastScreenBlitSupportPayload* out
      ) {
        if (out) out->Supported = false;
      }

      /**
       * @brief Returns the SharedBufferID of the VRAM framebuffer.
       * @param out Pointer to a `Payloads::FramebufferBufferIDPayload` to
       *        fill.
       */
      virtual void GetFramebufferBufferID(
        Payloads::FramebufferBufferIDPayload* out
      ) {
        if (out) out->BufferID = 0;
      }

      /**
       * @brief Acquires exclusive display ownership for a process.
       *
       * Transitions the driver to compositing mode. While a display owner
       * is set, text-mode operations (`WriteText`, etc.) are inhibited.
       *
       * @param ownerPID The @ref ProcessID of the acquiring process.
       * @return `true` if ownership was granted, false if the display is
       *         already owned by another process.
       */
      virtual bool AcquireDisplay(UInt32 ownerPID) {
        (void)ownerPID;

        return false;
      }

      /**
       * @brief Releases display ownership, reverting to text mode.
       *
       * If `ownerPID` does not match the current owner, the call is ignored.
       *
       * @param ownerPID The @ref ProcessID of the releasing process.
       */
      virtual void ReleaseDisplay(UInt32 ownerPID) {
        (void)ownerPID;
      }

      /**
       * @brief Queries the current display owner and compositing state.
       * @param out Pointer to a `Payloads::DisplayOwnerPayload` to fill.
       */
      virtual void GetDisplayOwner(Payloads::DisplayOwnerPayload* out) {
        if (out) {
          out->OwnerPID = 0;
          out->IsCompositing = false;
        }
      }

      /**
       * @brief Dispatches a driver operation by code.
       * @param operation The operation code (see GraphicsDriverOperation).
       * @param payload Operation-specific payload pointer.
       * @return 0 on success, non-zero on failure.
       */
      UInt32 Invoke(UInt32 operation, void* payload) override {
        switch (static_cast<GraphicsDriverOperation>(operation)) {
          case GraphicsDriverOperation::WriteText:
            WriteText(static_cast<const char*>(payload));

            return 0;

          case GraphicsDriverOperation::SetMode: {
            auto* p = static_cast<const Payloads::SetModePayload*>(payload);

            SetMode(p->Mode);

            return 0;
          }

          case GraphicsDriverOperation::FillRectangle: {
            auto* p = static_cast<const Payloads::FillRectanglePayload*>(
              payload
            );

            FillRectangle(p->X, p->Y, p->Width, p->Height, p->Color);

            return 0;
          }

          case GraphicsDriverOperation::BlitBuffer: {
            auto* p = static_cast<const Payloads::BlitBufferPayload*>(payload);

            BlitBuffer(
              p->X, p->Y, p->Width, p->Height,
              p->TransparentColor, p->Pixels
            );

            return 0;
          }

          case GraphicsDriverOperation::XORRectangle: {
            auto* p = static_cast<const Payloads::XORRectanglePayload*>(
              payload
            );

            XORRectangle(p->X, p->Y, p->Width, p->Height, p->Color);

            return 0;
          }

          case GraphicsDriverOperation::GetModeInfo: {
            auto* p = static_cast<Payloads::ModeInfoPayload*>(payload);

            GetModeInfo(p);

            return 0;
          }

          case GraphicsDriverOperation::SetBatchMode: {
            auto* p = static_cast<const Payloads::SetBatchModePayload*>(
              payload
            );

            SetBatchMode(p->Enabled);

            return 0;
          }

          case GraphicsDriverOperation::FlushRegion: {
            auto* p = static_cast<const Payloads::FlushRegionPayload*>(
              payload
            );

            FlushRegion(p->X, p->Y, p->Width, p->Height);

            return 0;
          }

          case GraphicsDriverOperation::ScreenBlit: {
            auto* p = static_cast<const Payloads::ScreenBlitPayload*>(payload);

            ScreenBlit(
              p->SourceX, p->SourceY,
              p->DestinationX, p->DestinationY,
              p->Width, p->Height
            );

            return 0;
          }

          case GraphicsDriverOperation::GetHardwareCursorSupport: {
            auto* p = static_cast<Payloads::HardwareCursorSupportPayload*>(
              payload
            );

            GetHardwareCursorSupport(p);

            return 0;
          }

          case GraphicsDriverOperation::SetHardwareCursorBitmap: {
            auto* p = static_cast<const Payloads::SetHardwareCursorBitmapPayload*>(
              payload
            );

            SetHardwareCursorBitmap(
              p->Width, p->Height, p->TransparentColor, p->Pixels
            );

            return 0;
          }

          case GraphicsDriverOperation::SetHardwareCursorPosition: {
            auto* p = static_cast<const Payloads::SetHardwareCursorPositionPayload*>(
              payload
            );

            SetHardwareCursorPosition(p->X, p->Y);

            return 0;
          }

          case GraphicsDriverOperation::SetHardwareCursorVisible: {
            auto* p = static_cast<const Payloads::SetHardwareCursorVisiblePayload*>(
              payload
            );

            SetHardwareCursorVisible(p->Visible);

            return 0;
          }

          case GraphicsDriverOperation::WritePixelRegion: {
            auto* p = static_cast<const Payloads::WritePixelRegionPayload*>(
              payload
            );

            WritePixelRegion(
              p->X, p->Y, p->Width, p->Height, p->SrcPitch, p->Pixels
            );

            return 0;
          }

          case GraphicsDriverOperation::GetFastScreenBlitSupport: {
            auto* p = static_cast<Payloads::FastScreenBlitSupportPayload*>(
              payload
            );

            GetFastScreenBlitSupport(p);

            return 0;
          }

          case GraphicsDriverOperation::WriteNativeRegion: {
            auto* p = static_cast<const Payloads::WriteNativeRegionPayload*>(
              payload
            );

            WriteNativeRegion(
              p->X, p->Y, p->Width, p->Height, p->SrcPitch, p->Pixels
            );

            return 0;
          }

          case GraphicsDriverOperation::GetFramebufferBufferID: {
            auto* p = static_cast<Payloads::FramebufferBufferIDPayload*>(
              payload
            );

            GetFramebufferBufferID(p);

            return 0;
          }

          case GraphicsDriverOperation::AcquireDisplay: {
            auto* p = static_cast<const Payloads::AcquireDisplayPayload*>(
              payload
            );

            return AcquireDisplay(p->OwnerPID) ? 0 : 1;
          }

          case GraphicsDriverOperation::ReleaseDisplay: {
            auto* p = static_cast<const Payloads::ReleaseDisplayPayload*>(
              payload
            );

            ReleaseDisplay(p->OwnerPID);

            return 0;
          }

          case GraphicsDriverOperation::GetDisplayOwner: {
            auto* p = static_cast<Payloads::DisplayOwnerPayload*>(payload);

            GetDisplayOwner(p);

            return 0;
          }

          default:
            return static_cast<UInt32>(-1);
        }
      }
  };
}
