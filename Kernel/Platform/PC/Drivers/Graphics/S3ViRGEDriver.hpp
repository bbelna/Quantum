/**
 * @file Kernel/Platform/PC/Drivers/Graphics/S3ViRGEDriver.hpp
 * @brief Declares @ref @QKrnlPC::Drivers::Graphics::S3ViRGEDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/HAL/Platform/PC/Bus/PCI.hpp>

#include <Concurrency/Spinlock.hpp>
#include <Drivers/DriverTypes.hpp>
#include <Drivers/Graphics/GraphicsDriverTypes.hpp>
#include <Platform/PC/Drivers/Bus/PCI.hpp>
#include <Platform/PC/Drivers/Graphics/PCGraphicsDriverTypes.hpp>

namespace Quantum::Kernel::Platform::PC::Drivers::Graphics {
  /**
   * @brief Kernel-level S3 ViRGE acceleration driver.
   *
   * This driver provides hardware-accelerated 2D operations using the S3
   * ViRGE's BLT engine via New MMIO registers (BAR0 + 16 MB).
   *
   * CR53 bit 3 (New MMIO) is enabled permanently during initialization.
   * The BLT engine executes asynchronously after registers are programmed.
   *
   * Text rendering is handled directly using an 8x14 bitmap font rendered
   * to both the shadow buffer and the linear framebuffer.
   *
   * Initialization is two-phase: the constructor probes PCI bus 0 for S3 vendor
   * ID `0x5333`, and `Initialize()` maps MMIO registers and unlocks S3
   * extensions.
   */
  class S3ViRGEDriver : public HAL::Graphics::GraphicsDriver {
    public:
      /**
       * @brief S3 ViRGE PCI device IDs to probe for during construction.
       */
      static constexpr UInt16 DeviceIDs[] = {
        0x5631,  // ViRGE (325)
        0x8A01,  // ViRGE/DX, ViRGE/GX
        0x8A10,  // ViRGE/GX2
        0x883D,  // ViRGE/VX
        0x8C01,  // ViRGE/MX
        0x8C03,  // ViRGE/MXC
      };

      /**
       * @brief Kernel virtual address for S3 ViRGE MMIO registers.
       */
      static constexpr UInt32 MMIOVirtualAddress = 0xE8400000;

      /**
       * @brief Size of the MMIO register region (64 KB).
       */
      static constexpr UInt32 MMIOSize = 0x10000;

      /**
       * @brief Offset from BAR0 to the New MMIO register space (16 MB).
       */
      static constexpr UInt32 MMIOOffset = 0x1000000;

      /**
       * @brief VESA framebuffer virtual address (shared with VESA driver).
       */
      static constexpr UInt32 FramebufferAddress= 0xE8000000;

      /**
       * @brief S3 PCI vendor ID.
       */
      static constexpr UInt16 S3VendorID = 0x5333;

      /**
       * @brief BLT engine register offset: subsystem status.
       */
      static constexpr UInt32 RegisterSubsystemStatus = 0x8504;

      /**
       * @brief BLT engine register offset: source base address.
       */
      static constexpr UInt32 RegisterSourceBase = 0xA4D4;

      /**
       * @brief BLT engine register offset: destination base address.
       */
      static constexpr UInt32 RegisterDestinationBase = 0xA4D8;

      /**
       * @brief BLT engine register offset: destination/source stride.
       */
      static constexpr UInt32 RegisterDestinationSourceStride = 0xA4E4;

      /**
       * @brief BLT engine register offset: pattern foreground color.
       */
      static constexpr UInt32 RegisterPatternForegroundColor = 0xA4F4;

      /**
       * @brief BLT engine register offset: command set.
       */
      static constexpr UInt32 RegisterCommandSet = 0xA500;

      /**
       * @brief BLT engine register offset: rectangle width and height.
       */
      static constexpr UInt32 RegisterRectangleWidthHeight = 0xA504;

      /**
       * @brief BLT engine register offset: source x/y-coordinates.
       */
      static constexpr UInt32 RegisterSourceXY = 0xA508;

      /**
       * @brief BLT engine register offset: destination x/y-coordinates.
       */
      static constexpr UInt32 RegisterDestinationXY = 0xA50C;

      /**
       * @brief CMD_SET command type: screen-to-screen BitBLT (bits 30:27).
       */
      static constexpr UInt32 CommandBitBlt = (0x0 << 27);

      /**
       * @brief CMD_SET command type: rectangle fill (bits 30:27).
       */
      static constexpr UInt32 CommandRectFill = (0x2 << 27);

      /**
       * @brief CMD_SET direction bit: left-to-right scan.
       */
      static constexpr UInt32 CommandXPositive = (1 << 25);

      /**
       * @brief CMD_SET direction bit: top-to-bottom scan.
       */
      static constexpr UInt32 CommandYPositive = (1 << 26);

      /**
       * @brief CMD_SET control bit: initiate drawing.
       */
      static constexpr UInt32 CommandDraw = (1 << 5);

      /**
       * @brief CMD_SET color format: 8 bits per pixel (bits 4:2).
       */
      static constexpr UInt32 Format8Bpp = (0 << 2);

      /**
       * @brief CMD_SET color format: 16 bits per pixel (bits 4:2).
       */
      static constexpr UInt32 Format16Bpp = (1 << 2);

      /**
       * @brief CMD_SET color format: 24 bits per pixel (bits 4:2).
       */
      static constexpr UInt32 Format24Bpp = (2 << 2);

      /**
       * @brief ROP code: source copy (bits 24:17).
       */
      static constexpr UInt32 RopSourceCopy = (0xCC << 17);

      /**
       * @brief CMD_SET control bit: disable block write acceleration.
       *        Keeping this set avoids direction-dependent corruption on
       *        ViRGE when BLTs are not strictly left-to-right.
       */
      static constexpr UInt32 CommandDisableBlockWrite = (1 << 16);

      /**
       * @brief ROP code: pattern copy (bits 24:17).
       */
      static constexpr UInt32 RopPatternCopy = (0xF0 << 17);

      /**
       * @brief ROP code: pattern XOR (bits 24:17).
       */
      static constexpr UInt32 RopPatternXor = (0x5A << 17);

      /**
       * @brief Status register bit: all engines idle (bit 13).
       *
       * On the S3 ViRGE, SUBSYS_STATUS bit 13 is SET when all 2D/3D
       * engines and the command FIFO are idle, and CLEAR when any
       * subsystem has pending work.
       */
      static constexpr UInt32 StatusEngineIdle = (1 << 13);

      /**
       * @brief Status register mask for available 2D command FIFO slots
       *        (bits 12:8).
       */
      static constexpr UInt32 StatusFifoFreeMask = (0x1F << 8);

      /**
       * @brief Status register shift for available FIFO slots.
       */
      static constexpr UInt32 StatusFifoFreeShift = 8;

      /**
       * @brief Size of the hardware cursor data in VRAM in bytes.
       *        64 rows x 16 bytes/row (interleaved AND/XOR planes).
       */
      static constexpr UInt32 CursorDataSize = 1024;

      /**
       * @brief Alignment requirement for the cursor VRAM base address.
       */
      static constexpr UInt32 CursorVramAlignment = 1024;

      /**
       * @brief Probes PCI bus 0 for an S3 ViRGE device matching known device
       *        IDs.
       * @param pci The PCI driver to use for probing.
       * @return `true` if a supported S3 ViRGE device was found; `false`
       *         otherwise.
       */
      static bool Discover(Bus::PCI* pci);

      /**
       * @brief Creates a new `S3ViRGE` instance.
       * @param pci The PCI driver to probe for the S3 ViRGE device.
       * @param cpu The CPU driver for I/O port access.
       * @param memoryAllocator The kernel memory allocator.
       * @param sharedBuffers The shared buffer registry.
       *
       * Probes PCI bus 0 for an S3 vendor ID (0x5333) and reads VESA mode
       * information from the E820BootInfo structure at physical address 0x8000.
       */
      explicit S3ViRGEDriver(
        Bus::PCI* pci,
        ICPUDriver* cpu,
        IMemoryAllocator* memoryAllocator,
        SharedBufferRepository* sharedBuffers
      );

      /**
       * @brief Destroys the `S3ViRGE` instance.
       */
      virtual ~S3ViRGEDriver() = default;

      /**
       * @brief Returns whether an S3 ViRGE device was found on the PCI bus.
       */
      bool IsFound() const { return _found; }

      /**
       * @brief Maps MMIO registers into kernel virtual address space and
       *        initializes the BLT engine.
       * @param mapper The virtual memory mapper to use for mapping.
       * @param addressSpace The kernel address space to map into.
       * @return `true` if initialization succeeded; `false` otherwise.
       */
      bool Initialize(
        IMemoryMapper* mapper,
        IAddressSpace* addressSpace
      );

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
       * @brief Writes a single character at the current cursor position.
       * @param character The character to write.
       */
      void WriteCharacter(char character);

      /**
       * @brief Writes a null-terminated string to the screen.
       * @param text The string to write.
       */
      void WriteText(const char* text) override;

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
       * @param mode Ignored.
       */
      void SetMode(UInt16 mode) override;

      /**
       * @brief Fills a rectangle using the BLT engine's PATCOPY ROP.
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
       * @brief XORs a rectangle using the BLT engine's PATXOR ROP.
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
       * @brief Moves a rectangular region of the screen using the BLT engine's
       *        SRCCOPY ROP with direction-aware overlap handling.
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
       * @brief Returns `true` to indicate S3 ViRGE hardware cursor support.
       * @param out Pointer to the support payload to fill.
       */
      void GetHardwareCursorSupport(
        HardwareCursorSupportPayload* out
      ) override;

      /**
       * @brief Returns `true` to indicate S3 ViRGE hardware-accelerated
       *        screen-to-screen BLT support via the 2D BLT engine.
       * @param out Pointer to the support payload to fill.
       */
      void GetFastScreenBlitSupport(
        FastScreenBlitSupportPayload* out
      ) override;

      /**
       * @brief Converts an ARGB cursor bitmap to the S3 AND/XOR plane format
       *        and writes it to the hardware cursor VRAM slot.
       * @param w Source bitmap width in pixels (max 64).
       * @param h Source bitmap height in pixels (max 64).
       * @param transparent Color treated as transparent.
       * @param pixels Source ARGB pixel data (`w * h` values).
       */
      void SetHardwareCursorBitmap(
        UInt8 w,
        UInt8 h,
        UInt32 transparent,
        const UInt32* pixels
      ) override;

      /**
       * @brief Writes the hardware cursor position to CRTC registers
       *        CR46/CR47 (X) and CR48/CR49 (Y).
       * @param x New x-coordinate (clamped to [0, 2047]).
       * @param y New y-coordinate (clamped to [0, 2047]).
       */
      void SetHardwareCursorPosition(Int16 x, Int16 y) override;

      /**
       * @brief Enables or disables the hardware cursor via CRTC register CR45
       *        bit 0.
       * @param visible `true` to show the cursor, false to hide it.
       */
      void SetHardwareCursorVisible(bool visible) override;

      /**
       * @brief Converts a rectangular region of ARGB32 pixels to RGB565 and
       *        writes them to the shadow buffer.
       * @param x Destination x-coordinate.
       * @param y Destination y-coordinate.
       * @param w Width of the region in pixels.
       * @param h Height of the region in pixels.
       * @param srcPitch Row width of the source buffer in pixels.
       * @param pixels Pointer to the ARGB32 source buffer. The region
       *        starts at `pixels[y * srcPitch + x]`.
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
       * @brief Copies a rectangular region of pixels already in native RGB565
       *        format to the shadow buffer.
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
       * @brief Pointer to the PCI driver for probing and configuration.
       */
      Bus::PCI* _pci = nullptr;

      /**
       * @brief Pointer to the CPU driver for I/O port access.
       */
      ICPUDriver* _cpu = nullptr;

      /**
       * @brief The kernel memory allocator.
       */
      IMemoryAllocator* _memoryAllocator = nullptr;

      /**
       * @brief The shared buffer registry.
       */
      SharedBufferRepository* _sharedBuffers = nullptr;

      /**
       * @brief Whether an S3 ViRGE device was found on the PCI bus.
       */
      bool _found = false;

      /**
       * @brief Whether the driver has been fully initialized.
       */
      bool _initialized = false;

      /**
       * @brief VRAM byte offset of the hardware cursor data (1 KB aligned,
       *        immediately after the visible framebuffer). Set in Initialize().
       */
      UInt32 _cursorVramBase = 0;

      /**
       * @brief Whether batch mode is active (shadow-only writes).
       */
      bool _batchMode = false;

      /**
       * @brief Whether deferred BlitBuffer writes exist in the shadow buffer.
       */
      bool _hasDeferredWrites = false;

      /**
       * @brief @ref ProcessID of the current display owner, or 0 if none.
       *
       * When non-zero the driver is in compositing mode and text-mode
       * operations are inhibited. Protected by `_lock`.
       */
      UInt32 _displayOwnerPID = 0;

      /**
       * @brief Pointer to the mapped MMIO register space.
       */
      volatile UInt8* _mmio = nullptr;

      /**
       * @brief Pointer to the mapped linear framebuffer.
       */
      volatile UInt8* _framebuffer = nullptr;

      /**
       * @brief Shadow copy of the framebuffer in cached RAM.
       */
      UInt8* _shadow = nullptr;

      /**
       * @brief PCI BAR0 base address (physical).
       */
      UInt32 _bar0 = 0;

      /**
       * @brief Shared buffer ID for the framebuffer, used by the graphics
       *        server for zero-copy access.
       */
      SharedBufferID _framebufferBufferID = 0;

      /**
       * @brief Display width in pixels.
       */
      UInt16 _width = 0;

      /**
       * @brief Display height in pixels.
       */
      UInt16 _height = 0;

      /**
       * @brief Bytes per scanline / pitch.
       */
      UInt32 _pitch = 0;

      /**
       * @brief Bits per pixel.
       */
      UInt8 _bpp = 0;

      /**
       * @brief Bytes per pixel (bpp / 8).
       */
      UInt8 _bytesPerPixel = 0;

      /**
       * @brief CR53 register value with MMIO disabled (base for toggling).
       */
      UInt8 _cr53Base = 0;

      /**
       * @brief The device associated with this driver.
       */
      Device _device {
        5,
        "S3ViRGE",
        "S3 ViRGE Graphics",
        ToDeviceCategoryID(DeviceCategoryType::Graphics),
        DeviceState::Discovered,
        0,
        0,
        DeviceBus::PCI,
        0,
        {}
      };

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
       * @brief Spinlock for synchronizing access to the BLT engine and
       *        framebuffer.
       */
      Spinlock<UInt32> _lock;

      /**
       * @brief Scratch buffer for building the 64x64 hardware cursor bitmap.
       *        Kept as a member to avoid a 1 KB stack allocation in
       *        `SetHardwareCursorBitmap`. Access is serialized by `_lock`.
       */
      UInt8 _cursorBuffer[CursorDataSize];

      /**
       * @brief Converts a 32-bit ARGB color to RGB565 format.
       * @param argb The 32-bit ARGB color value.
       * @return The 16-bit RGB565 color value.
       */
      static UInt16 _toRGB565(UInt32 argb) {
        return static_cast<UInt16>(
          ((argb >> 8) & 0xF800) |
          ((argb >> 5) & 0x07E0) |
          ((argb >> 3) & 0x001F)
        );
      }

      /**
       * @brief Writes a 32-bit value to an MMIO register.
       * @param offset The register offset within the MMIO region.
       * @param value The value to write.
       */
      void _mmioWrite32(UInt32 offset, UInt32 value);

      /**
       * @brief Reads a 32-bit value from an MMIO register.
       * @param offset The register offset within the MMIO region.
       * @return The register value.
       */
      UInt32 _mmioRead32(UInt32 offset);

      /**
       * @brief Enables MMIO access by setting CR53 bit 3.
       */
      void _enableMMIO();

      /**
       * @brief Disables MMIO access by clearing CR53 bit 3.
       */
      void _disableMMIO();

      /**
       * @brief Spins until the BLT engine is idle.
       */
      void _waitIdle();

      /**
       * @brief Waits until at least `entries` command FIFO slots are free.
       * @param entries Number of 32-bit MMIO command writes to queue.
       */
      void _waitFifo(UInt8 entries);

      /**
       * @brief Unlocks S3 sequencer and CRTC extension registers.
       */
      void _unlockRegisters();

      /**
       * @brief Writes a single character at the current cursor position.
       * @param ch The character to write.
       */
      void _writeCharacter(char ch);

      /**
       * @brief Renders an 8x14 font glyph at the given text grid position.
       * @param col The text column.
       * @param row The text row.
       * @param ch The character to render.
       */
      void _renderGlyph(UInt16 col, UInt16 row, char ch);

      /**
       * @brief Scrolls the screen up by one text row (14 pixels).
       */
      void _scrollUp();

      /**
       * @brief Draws the block cursor at the current cursor position.
       */
      void _drawTextCursor();

      /**
       * @brief Hides the block cursor by re-rendering the character under it.
       */
      void _hideTextCursor();

      /**
       * @brief Returns the 32-bit `rep stosl` fill pattern that paints
       *        @ref ClearColor in the current pixel format. See the
       *        equivalent helper on @ref VESADriver for the per-bpp
       *        derivation.
       */
      UInt32 _clearPattern() const;
  };
}
