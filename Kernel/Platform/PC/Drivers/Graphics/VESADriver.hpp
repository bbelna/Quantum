/**
 * @file Kernel/Platform/PC/Drivers/Graphics/VESADriver.hpp
 * @brief Declares @ref @QKrnlPC::Drivers::Graphics::VESADriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/HAL/Platform/PC/Bus/ISA.hpp>

#include <Concurrency/Spinlock.hpp>
#include <Drivers/DriverTypes.hpp>
#include <Drivers/Graphics/GraphicsDriverTypes.hpp>
#include <Platform/PC/Drivers/Bus/PCI.hpp>
#include <Platform/PC/Drivers/Graphics/PCGraphicsDriverTypes.hpp>

namespace Quantum::Kernel::Platform::PC::Drivers::Graphics {
  using namespace Quantum::HAL::Graphics::Payloads;

  /**
   * @brief Kernel-level VESA (VESA BIOS Extensions) graphics driver.
   *
   * This driver uses the linear framebuffer set up by the bootloader via VESA
   * mode 0x118 (or similar). It provides both software text rendering for
   * kernel logging and hardware-accelerated (LFB-direct) graphics operations
   * for the graphics server pipeline.
   *
   * Initialization is two-phase:
   *   1. Constructor reads VESA mode info from BootInfo (identity-mapped).
   *   2. `MapFramebuffer()` must be called after memory management init to
   *      map the physical LFB into kernel virtual address space.
   *
   * Until `MapFramebuffer()` is called, `WriteCharacter()` and all graphics
   * operations silently no-op. Serial logging captures all messages from boot.
   */
  class VESADriver : public GraphicsDriver {
    public:
      /**
       * @brief The kernel virtual address where the LFB is mapped.
       */
      static constexpr UInt32 FramebufferAddress = 0xE8000000;

      /**
       * @brief Creates a new `VESA` instance.
       * @param maydayHandler The kernel mayday handler.
       * @param log The kernel log instance.
       * @param memoryAllocator The kernel memory allocator.
       * @param sharedBuffers The shared buffer registry.
       *
       * Reads VESA mode information from the E820BootInfo structure at
       * physical address 0x8000 (identity-mapped). Does NOT access the
       * framebuffer.
       */
      /**
       * @brief Creates a new `VESA` instance with no dependencies.
       *
       * Only reads VESA mode info from boot structures. Call
       * @ref SetDependencies before using framebuffer operations.
       */
      VESADriver();

      /**
       * @brief Injects runtime dependencies after kernel subsystems are ready.
       * @param maydayHandler Pointer to the mayday handler.
       * @param memoryAllocator Pointer to the kernel memory allocator.
       * @param sharedBuffers Pointer to the shared buffer registry.
       */
      void SetDependencies(
        IMaydayHandler* maydayHandler,
        IMemoryAllocator* memoryAllocator,
        SharedBufferRepository* sharedBuffers
      );

      /**
       * @brief Destroys the `VESA` instance.
       */
      virtual ~VESADriver() = default;

      /**
       * @brief Gets the device associated with this driver.
       * @return The device associated with this driver.
       */
      Device& GetDevice() override { return _device; }

      /**
       * @brief Gets the unique identifier of the device associated with this
       *        driver.
       * @return The unique identifier of the device.
       */
      DeviceID GetDeviceID() override { return _device.ID; }

      /**
       * @brief Maps the VESA linear framebuffer into kernel virtual address
       *        space and clears the screen.
       * @param mapper The virtual memory mapper to use for mapping.
       * @param addressSpace The kernel address space to map into.
       *
       * Must be called after memory management is initialized. After this
       * call, `WriteCharacter()` and all graphics operations become active.
       */
      void MapFramebuffer(
        IMemoryMapper* mapper,
        IAddressSpace* addressSpace
      );

      /**
       * @brief Writes a character to the screen at the current text cursor
       *        position using software font rendering.
       * @param character The character to write.
       */
      void WriteCharacter(char character);

      /**
       * @brief Writes a null-terminated string to the screen starting at the
       *        current text cursor position.
       * @param string The string to write.
       */
      void WriteText(const char* string) override;

      /**
       * @brief Sets the foreground color for subsequent text rendering.
       * @param color The 32-bit ARGB color value.
       */
      void SetTextForegroundColor(UInt32 color) override;

      /**
       * @brief Gets the current text cursor position.
       * @return The current text cursor position (X = column, Y = row).
       */
      Geometry2D::Point GetTextCursorPosition() override;

      /**
       * @brief Sets the text cursor position.
       * @param position The new text cursor position (X = column, Y = row).
       */
      void SetTextCursorPosition(Geometry2D::Point position) override;

      /**
       * @brief No-op. The VESA mode is configured by the bootloader.
       * @param mode Ignored. The active mode info is returned by
       *        `GetModeInfo()`.
       */
      void SetMode(UInt16 mode) override;

      /**
       * @brief Fills a rectangle with a solid 32-bit ARGB color.
       * @param x The x-coordinate of the top-left corner.
       * @param y The y-coordinate of the top-left corner.
       * @param w The width in pixels.
       * @param h The height in pixels.
       * @param color The 32-bit ARGB color value.
       */
      void FillRectangle(
        UInt16 x,
        UInt16 y,
        UInt16 w,
        UInt16 h,
        UInt32 color
      ) override;

      /**
       * @brief Copies a pixel buffer to the framebuffer, skipping transparent
       *        pixels.
       * @param x The x-coordinate of the top-left corner.
       * @param y The y-coordinate of the top-left corner.
       * @param w The width in pixels.
       * @param h The height in pixels.
       * @param transparentColor Pixels matching this value are skipped.
       * @param pixels The pixel data (`w` * `h` 32-bit ARGB values).
       */
      void BlitBuffer(
        UInt16 x,
        UInt16 y,
        UInt16 w,
        UInt16 h,
        UInt32 transparentColor,
        const UInt32* pixels
      ) override;

      /**
       * @brief XORs a rectangle with a 32-bit ARGB color.
       * @param x The x-coordinate of the top-left corner.
       * @param y The y-coordinate of the top-left corner.
       * @param w The width in pixels.
       * @param h The height in pixels.
       * @param color The 32-bit ARGB color value to XOR with.
       */
      void XORRectangle(
        UInt16 x,
        UInt16 y,
        UInt16 w,
        UInt16 h,
        UInt32 color
      ) override;

      /**
       * @brief Queries the current video mode information.
       * @param info Pointer to a `ModeInfoPayload` struct to fill.
       */
      void GetModeInfo(ModeInfoPayload* info) override;

      /**
       * @brief Enables or disables batch mode (shadow-only writes).
       * @param enabled `true` to enable, false to disable.
       */
      void SetBatchMode(bool enabled) override;

      /**
       * @brief Copies a region from the shadow buffer to the framebuffer.
       * @param x The x-coordinate of the region's top-left corner.
       * @param y The y-coordinate of the region's top-left corner.
       * @param w The width of the region in pixels.
       * @param h The height of the region in pixels.
       */
      void FlushRegion(
        UInt16 x,
        UInt16 y,
        UInt16 w,
        UInt16 h
      ) override;

      /**
       * @brief Moves a rectangular region of the screen to a new position
       *        with direction-aware overlap handling.
       * @param srcX The x-coordinate of the source region's top-left corner.
       * @param srcY The y-coordinate of the source region's top-left corner.
       * @param dstX The x-coordinate of the destination region's top-left
       *        corner.
       * @param dstY The y-coordinate of the destination region's top-left
       *        corner.
       * @param w The width of the region in pixels.
       * @param h The height of the region in pixels.
       */
      void ScreenBlit(
        UInt16 srcX,
        UInt16 srcY,
        UInt16 dstX,
        UInt16 dstY,
        UInt16 w,
        UInt16 h
      ) override;

      /**
       * @brief Converts an ARGB32 pixel region to the native pixel format and
       *        writes it to the shadow buffer.
       * @param x Destination x-coordinate.
       * @param y Destination y-coordinate.
       * @param w Width of the region in pixels.
       * @param h Height of the region in pixels.
       * @param srcPitch Row width of the source buffer in pixels.
       * @param pixels Pointer to the ARGB32 source buffer; the region starts
       *        at `pixels[y * srcPitch + x]`.
       */
      void WritePixelRegion(
        UInt16 x,
        UInt16 y,
        UInt16 w,
        UInt16 h,
        UInt32 srcPitch,
        const UInt32* pixels
      ) override;

      /**
       * @brief Copies a rectangular region of pixels already in the native
       *        framebuffer format to the shadow buffer.
       * @param x Destination x-coordinate.
       * @param y Destination y-coordinate.
       * @param w Width of the region in pixels.
       * @param h Height of the region in pixels.
       * @param srcPitch Row width of the source buffer in pixels.
       * @param pixels Pointer to the native-format source buffer.
       */
      void WriteNativeRegion(
        UInt16 x,
        UInt16 y,
        UInt16 w,
        UInt16 h,
        UInt32 srcPitch,
        const void* pixels
      ) override;

      /**
       * @brief Returns the shared buffer ID backing the framebuffer.
       * @param out Pointer to the payload struct to fill with the buffer ID.
       */
      void GetFramebufferBufferID(
        FramebufferBufferIDPayload* out
      ) override;

      /**
       * @brief Acquires exclusive display ownership for a process.
       * @param ownerPID The @ref ProcessID of the acquiring process.
       * @return `true` if ownership was granted; `false` if the display is
       *         already owned by another process.
       */
      bool AcquireDisplay(UInt32 ownerPID) override;

      /**
       * @brief Releases display ownership, reverting to text mode.
       * @param ownerPID The @ref ProcessID of the releasing process.
       */
      void ReleaseDisplay(UInt32 ownerPID) override;

      /**
       * @brief Queries the current display owner and compositing state.
       * @param out Pointer to a `DisplayOwnerPayload` to fill.
       */
      void GetDisplayOwner(DisplayOwnerPayload* out) override;

    private:
      /**
       * @brief The kernel mayday handler.
       */
      IMaydayHandler* _maydayHandler = nullptr;

      /**
       * @brief The kernel memory allocator.
       */
      IMemoryAllocator* _memoryAllocator = nullptr;

      /**
       * @brief The shared buffer registry.
       */
      SharedBufferRepository* _sharedBuffers = nullptr;

      /**
       * @brief The device associated with this driver.
       */
      Device _device {
        1,
        "VESA",
        "VESA Graphics",
        ToDeviceCategoryID(DeviceCategoryType::Graphics),
        DeviceState::Active,
        0,
        0,
        DeviceBus::ISA,
        0,
        {}
      };

      /**
       * @brief Display width in pixels (from BootInfo).
       */
      UInt16 _width = 0;

      /**
       * @brief Display height in pixels (from BootInfo).
       */
      UInt16 _height = 0;

      /**
       * @brief Bytes per scanline / pitch (from BootInfo).
       */
      UInt32 _pitch = 0;

      /**
       * @brief Bits per pixel (from BootInfo).
       */
      UInt8 _bpp = 0;

      /**
       * @brief Physical address of the linear framebuffer.
       */
      UInt32 _fbPhysical = 0;

      /**
       * @brief Size of the linear framebuffer in bytes.
       */
      UInt32 _fbSize = 0;

      /**
       * @brief Shared buffer ID for the framebuffer, used by the graphics
       *        server for zero-copy access.
       */
      SharedBufferID _framebufferBufferID = 0;

      /**
       * @brief Pointer to the mapped linear framebuffer (byte-level). Null
       *        until `MapFramebuffer()` is called.
       */
      volatile UInt8* _framebuffer = nullptr;

      /**
       * @brief Shadow copy of the framebuffer in cached RAM. Avoids slow
       *        uncached MMIO reads during scroll operations.
       */
      UInt8* _shadow = nullptr;

      /**
       * @brief Bytes per pixel (3 for 24bpp, 4 for 32bpp).
       */
      UInt8 _bytesPerPixel = 0;

      /**
       * @brief Number of text columns (width / 8).
       */
      UInt16 _textColumns = 0;

      /**
       * @brief Number of text rows (height / 14).
       */
      UInt16 _textRows = 0;

      /**
       * @brief Current text cursor column.
       */
      UInt16 _cursorCol = 0;

      /**
       * @brief Current text cursor row.
       */
      UInt16 _cursorRow = 0;

      /**
       * @brief Text foreground color (white).
       */
      UInt32 _textFg = 0xFFFFFFFF;

      /**
       * @brief Text background color, matched to @ref ClearColor so the
       *        glyph backgrounds blend with the cleared framebuffer.
       */
      UInt32 _textBg = ClearColor;

      /**
       * @brief The character currently under the text cursor.
       */
      char _cursorChar = ' ';

      /**
       * @brief Whether the block cursor is currently drawn.
       */
      bool _cursorDrawn = false;

      /**
       * @brief When `true`, drawing operations update the shadow buffer only
       *        and skip the slow copy to the UC framebuffer. Call
       *        `FlushRegion()` to copy a region from shadow to framebuffer.
       */
      bool _batchMode = false;

      /**
       * @brief @ref ProcessID of the current display owner, or 0 if none.
       *
       * When non-zero the driver is in compositing mode and text-mode
       * operations are inhibited. Protected by `_lock`.
       */
      UInt32 _displayOwnerPID = 0;

      /**
       * @brief Spinlock for synchronizing access to the framebuffer.
       */
      Spinlock<UInt32> _lock;

      /**
       * @brief Renders a single glyph at the specified text grid position.
       * @param col The text column.
       * @param row The text row.
       * @param ch The character to render.
       */
      void _renderGlyph(UInt16 col, UInt16 row, char ch);

      /**
       * @brief Scrolls the framebuffer content up by one text row (16 pixels).
       */
      void _scrollUp();

      /**
       * @brief Draws the block cursor at the current text position.
       */
      void _drawCursor();

      /**
       * @brief Hides the block cursor, restoring the character underneath.
       */
      void _hideCursor();

      /**
       * @brief Writes a pixel to the framebuffer at the given coordinates.
       * @param x The X pixel coordinate.
       * @param y The Y pixel coordinate.
       * @param color The 32-bit ARGB color value.
       */
      void _putPixel(UInt16 x, UInt16 y, UInt32 color);

      /**
       * @brief Reads a pixel from the framebuffer at the given coordinates.
       * @param x The X pixel coordinate.
       * @param y The Y pixel coordinate.
       * @return The 32-bit ARGB color value.
       */
      UInt32 _getPixel(UInt16 x, UInt16 y);

      /**
       * @brief Returns the 32-bit `rep stosl` fill pattern that paints
       *        @ref ClearColor in the current pixel format.
       *
       * For 32bpp the pattern is the ARGB value itself; for 16bpp it
       * is the RGB565 conversion packed twice into a dword; for 24bpp
       * it is the byte `0x10` repeated four times (correct since
       * R = G = B in @ref ClearColor).
       */
      UInt32 _clearPattern() const;
  };
}
