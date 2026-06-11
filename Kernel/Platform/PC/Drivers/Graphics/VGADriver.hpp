/**
 * @file Kernel/Platform/PC/Drivers/Graphics/VGADriver.hpp
 * @brief Declares @ref @QKrnlPC::Drivers::Graphics::VGADriver.
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

namespace Quantum::Kernel::Platform::PC::Drivers::Graphics {
  /**
   * @brief Describes the state of the VGA text-mode cursor.
   */
  struct VGACursorState {
    /**
     * @brief The current row of the cursor.
     */
    UInt8 Row;

    /**
     * @brief The current column of the cursor.
     */
    UInt8 Column;

    /**
     * @brief The saved cell value under the cursor.
     */
    UInt16 Cell;

    /**
     * @brief Whether the cursor is currently visible.
     */
    bool Visible;
  };

  /**
   * @brief Kernel-level VGA driver.
   *
   * Supports both the standard 80x25 text mode (via the 0xB8000 buffer)
   * and two graphics modes: Mode 12h (640x480, 16-color planar) and
   * Mode 13h (320x200, 256-color linear at 0xA0000). Text output uses a
   * software cursor that is drawn and hidden as the cursor position changes.
   */
  class VGADriver : public GraphicsDriver {
    public:
      /**
       * @brief Creates a new `VGA` instance.
       * @param cpu The CPU driver for I/O port access.
       * @param maydayHandler The kernel mayday handler.
       */
      explicit VGADriver(ICPUDriver& cpu, IMaydayHandler* maydayHandler);

      /**
       * @brief Destroys the `VGA` instance.
       */
      virtual ~VGADriver() = default;

      /**
       * @brief Gets the device associated with this driver.
       * @return The device associated with this driver.
       */
      Device& GetDevice() override { return _device; }

      /**
       * @brief Gets the unique identifier of the device associated with this
       *        driver.
       * @return The unique identifier of the device associated with this
       *         driver.
       */
      DeviceID GetDeviceID() override { return _device.ID; }

      /**
       * @brief Writes a character to the VGA text-mode buffer at the current
       *        cursor position.
       * @param character The character to write.
       */
      void WriteCharacter(char character);

      /**
       * @brief Writes a null-terminated string to the VGA text-mode buffer
       *        starting at the current cursor position.
       * @param string The string to write.
       */
      void WriteText(const char* string) override;

      /**
       * @brief Clears the VGA text-mode display.
       */
      void Clear();

      /**
       * @brief Sets the foreground and background colors for subsequent
       *        text output.
       * @param foreground The foreground color (0-15).
       * @param background The background color (0-15).
       */
      void SetColor(UInt8 foreground, UInt8 background);

      /**
       * @brief Sets the text cursor position.
       * @param position The new text cursor position. 
       */
      void SetTextCursorPosition(Geometry2D::Point position) override;

      /**
       * @brief Gets the current text cursor position.
       * @return The current text cursor position.
       */
      Geometry2D::Point GetTextCursorPosition() override {
        return {
          static_cast<Int16>(_cursorState.Column),
          static_cast<Int16>(_cursorState.Row)
        };
      }

      /**
       * @brief Switches the display to the specified video mode.
       * @param mode The video mode number (0x12 or 0x13).
       */
      void SetMode(UInt16 mode) override;

      /**
       * @brief Fills a rectangle with a solid color in graphics mode.
       * @param x The x-coordinate of the top-left corner.
       * @param y The y-coordinate of the top-left corner.
       * @param w The width in pixels.
       * @param h The height in pixels.
       * @param color The 32-bit ARGB color value (low byte used as VGA palette
       *        index).
       */
      void FillRectangle(
        UInt16 x, UInt16 y, UInt16 w, UInt16 h, UInt32 color
      ) override;

      /**
       * @brief Copies a pixel buffer to the framebuffer in graphics mode.
       * @param x The x-coordinate of the top-left corner.
       * @param y The y-coordinate of the top-left corner.
       * @param w The width in pixels.
       * @param h The height in pixels.
       * @param transparentColor Pixels matching this value are skipped.
       * @param pixels The pixel data (`w` * `h` 32-bit ARGB values).
       */
      void BlitBuffer(
        UInt16 x, UInt16 y, UInt16 w, UInt16 h,
        UInt32 transparentColor, const UInt32* pixels
      ) override;

      /**
       * @brief XORs a rectangle with a color in graphics mode.
       * @param x The x-coordinate of the top-left corner.
       * @param y The y-coordinate of the top-left corner.
       * @param w The width in pixels.
       * @param h The height in pixels.
       * @param color The 32-bit ARGB color value to XOR with (low byte used
       *        as VGA palette index).
       */
      void XORRectangle(
        UInt16 x, UInt16 y, UInt16 w, UInt16 h, UInt32 color
      ) override;

      /**
       * @brief Queries the current video mode information.
       * @param info Pointer to a `ModeInfoPayload` struct to fill.
       */
      void GetModeInfo(ModeInfoPayload* info) override;

    private:
      /**
       * @brief The number of text-mode columns.
       */
      static constexpr UInt8 _columns = 80;

      /**
       * @brief The number of text-mode rows.
       */
      static constexpr UInt8 _rows = 25;

      /**
       * @brief The device associated with this driver.
       */
      Device _device {
        1,
        "VGA",
        "VGA Graphics",
        ToDeviceCategoryID(DeviceCategoryType::Graphics),
        DeviceState::Active,
        0,
        0,
        DeviceBus::ISA,
        0,
        {}
      };

      /**
       * @brief The text color (white on black).
       */
      UInt8 _color = 0x0F;

      /**
       * @brief The VGA text-mode buffer.
       */
      volatile UInt16* const _buffer
        = reinterpret_cast<volatile UInt16*>(0xB8000);

      /**
       * @brief The VGA Mode 13h linear framebuffer.
       */
      volatile UInt8* const _framebuffer
        = reinterpret_cast<volatile UInt8*>(0xA0000);

      /**
       * @brief Whether the display is currently in graphics mode.
       */
      bool _graphicsMode = false;

      /**
       * @brief Whether the display is in planar mode (Mode 12h) vs linear
       *        (Mode 13h).
       */
      bool _planarMode = false;

      /**
       * @brief The width of the graphics mode display in pixels.
       */
      UInt16 _gfxWidth = 320;

      /**
       * @brief The height of the graphics mode display in pixels.
       */
      UInt16 _gfxHeight = 200;

      /**
       * @brief The number of bytes per row in the framebuffer.
       */
      UInt16 _bytesPerRow = 320;

      /**
       * @brief The current state of the VGA cursor.
       */
      VGACursorState _cursorState = { 0, 0, false };

      /**
       * @brief The CPU driver for I/O port access.
       */
      ICPUDriver& _cpu;

      /**
       * @brief The kernel mayday handler.
       */
      IMaydayHandler* _maydayHandler = nullptr;

      /**
       * @brief Spinlock for synchronizing access to the VGA buffer and cursor
       *        state.
       */
      Spinlock<UInt32> _lock;

      /**
       * @brief Calculates the linear index in the VGA buffer for the given row
       *        and column.
       * @param row The row.
       * @param column The column.
       * @return The linear index calculated from the row and column.
       */
      UInt16 _index(UInt8 row, UInt8 column) {
        return static_cast<UInt16>(row * _columns + column);
      }

      /**
       * @brief Creates a VGA text-mode entry from a character and color.
       * @param character The character.
       * @param color The color attribute.
       * @return The VGA text-mode entry.
       */
      UInt16 _makeEntry(char character, UInt8 color) {
        return static_cast<UInt16>(character) |
              (static_cast<UInt16>(color) << 8);
      }

      /**
       * @brief Converts a 32-bit ARGB color value to the nearest VGA palette
       *        index. For VGA modes, the low byte is used as a direct palette
       *        index.
       * @param color The 32-bit ARGB color value.
       * @return The nearest VGA palette index.
       */
      UInt8 _toPaletteIndex(UInt32 color) {
        return static_cast<UInt8>(color & 0xFF);
      }

      /**
       * @brief Programs the VGA registers for the specified video mode.
       * @param miscOutput The Miscellaneous Output Register value.
       * @param seq The Sequencer register values (5 bytes).
       * @param crtc The CRTC register values (25 bytes).
       * @param gc The Graphics Controller register values (9 bytes).
       * @param ac The Attribute Controller register values (21 bytes).
       */
      void _programVGARegisters(
        UInt8 miscOutput,
        const UInt8* seq,
        const UInt8* crtc,
        const UInt8* gc,
        const UInt8* ac
      );

      /**
       * @brief Disables the hardware cursor by setting the appropriate bits
       *        in the VGA controller registers.
       */
      void _disableHardwareCursor();

      /**
       * @brief Draws the cursor.
       */
      void _drawCursor();

      /**
       * @brief Hides the cursor.
       */
      void _hideCursor();
  };
}
