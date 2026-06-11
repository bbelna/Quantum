/**
 * @file Include/Quantum/Components/Console.hpp
 * @brief Declares @ref Quantum::UI::Console and related structures.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Geometry2D.hpp>
#include <Quantum/Input.hpp>
#include <Quantum/Core/Types.hpp>
#include <Quantum/UI/Element.hpp>

namespace Quantum::Components {
  class Console;

  /**
   * @brief Context for a terminal command.
   */
  struct ConsoleCommandContext {
    /**
     * @brief The number of command arguments.
     */
    UInt32 argumentCount;

    /**
     * @brief The command's arguments.
     */
    const char** arguments;

    /**
     * @brief The command string.
     */
    const char* line;
  };

  /**
   * @brief Type of a terminal command handler function.
   * @param terminal The terminal instance.
   * @param context The command context.
   */
  using ConsoleCommandHandler = void (*)(
    Console& terminal,
    const ConsoleCommandContext& context
  );

  /**
   * @brief Callback invoked when the terminal has rendered new content
   *        and needs the display updated.
   * @param context Opaque pointer passed to the callback.
   * @param dirtyRect The region of the surface that was modified.
   * @param shiftDeltaY Pixel shift hint (positive = up). `0` if no shift.
   */
  using ConsoleInvalidateCallback = void (*)(
    void* context,
    Geometry2D::Rectangle dirtyRect,
    Int16 shiftDeltaY
  );

  /**
   * @brief Entry for a registered terminal command.
   */
  struct ConsoleCommandEntry {
    /**
     * @brief The command name.
     */
    const char* name;

    /**
     * @brief The command handler function.
     */
    ConsoleCommandHandler handler;
  };

  /**
   * @brief A text-mode terminal that renders a character grid into a
   *        @ref Canvas.
   *
   * Uses a fixed scrollback buffer allocated at the maximum possible width
   * (derived from the surface stride). On resize, only the visible lines change
   *, the buffer is never reallocated, so content is preserved across
   * resize-down-then-up cycles.
   *
   * Supports `Write`/`WriteLine`/`Clear`, auto-scrolling, cursor tracking, and
   * a command registration system. Input events are delivered externally
   * via `ProcessKeyEvent`.
   */
  class Console : public UI::Element {
    public:
      /**
       * @brief Default line spacing in pixels.
       */
      static constexpr UInt8 DefaultLineSpacing = 2;

      /**
       * @brief Default cursor width in pixels.
       */
      static constexpr UInt8 DefaultCursorWidth = 8;

      /**
       * @brief Tab stop interval in pixels.
       */
      static constexpr UInt16 TabStopPixels = 64;

      /**
       * @brief Sentinel byte marking a pixel-column jump in the buffer.
       *
       * The encoding is: `TabSentinel`, `lowByte`, `highByte` where the
       * two payload bytes form a little-endian `UInt16` pixel column.
       * The renderer jumps its X cursor to that pixel position instead
       * of drawing a glyph.
       */
      static constexpr char TabSentinel = '\x01';

      /**
       * @brief Default foreground color (opaque black).
       */
      static constexpr UInt32 DefaultForeground = 0xFF000000;

      /**
       * @brief Default background color (opaque white).
       */
      static constexpr UInt32 DefaultBackground = 0xFFFFFFFF;

      /**
       * @brief Maximum columns in the scrollback buffer.
       */
      static constexpr UInt16 MaxColumns = 240;

      /**
       * @brief Maximum rows in the scrollback buffer.
       */
      static constexpr UInt16 MaxBufferRows = 2000;

      /**
       * @brief Maximum number of characters in a single input line.
       */
      static constexpr Size MaxLineLength = 256;

      /**
       * @brief Maximum prompt length supported.
       */
      static constexpr Size MaxPromptLength = 64;

      /**
       * @brief Maximum number of registered commands.
       */
      static constexpr Size MaxCommands = 16;

      /**
       * @brief Maximum number of command arguments.
       */
      static constexpr Size MaxArguments = 16;

      /**
       * @brief Creates a new @ref Console instance.
       * @param surface The rendering surface, represented by a @ref Canvas
       *                instance.
       */
      explicit Console(UI::Canvas& surface);

      /**
       * @brief Destroys this @ref Console instance.
       */
      ~Console() override;

      /**
       * @brief Writes text to this terminal at the current cursor position.
       * @param text The null-terminated text to write.
       */
      void Write(const char* text);

      /**
       * @brief Writes a line of text to this terminal followed by a newline.
       * @param text The null-terminated text to write.
       */
      void WriteLine(const char* text);

      /**
       * @brief Advances the cursor to a target pixel column by emitting
       *        spaces. No-op if the cursor is already at or past the
       *        target.
       * @param pixelColumn Target pixel column.
       */
      void TabTo(UInt16 pixelColumn);

      /**
       * @brief Clears this terminal and resets the cursor.
       */
      void Clear();

      /**
       * @brief Sets the prompt string displayed by this terminal before each
       *        input line.
       * @param prompt The null-terminated prompt string.
       */
      void SetPrompt(const char* prompt);

      /**
       * @brief Registers a command with this terminal.
       * @param name The command name.
       * @param handler The command handler function.
       */
      void RegisterCommand(const char* name, ConsoleCommandHandler handler);

      /**
       * @brief Sets this terminal's default handler for unrecognized commands.
       * @param handler The fallback handler function.
       */
      void SetDefaultHandler(ConsoleCommandHandler handler);

      /**
       * @brief Sets a callback invoked after @ref Console::Draw when new
       *        content has been rendered.
       * @param callback The invalidation function.
       * @param context Opaque pointer passed to the callback.
       *
       * Used to push the dirty region to the display.
       */
      void SetInvalidateCallback(
        ConsoleInvalidateCallback callback,
        void* context
      );

      /**
       * @brief Executes a command.
       * @param command The command line string.
       */
      void Execute(const char* command);

      /**
       * @brief Processes a single keyboard input event.
       * @param event The input event to process.
       * @return `true` if the event was consumed; `false` otherwise.
       */
      bool ProcessKeyEvent(const Input::InputEvent& event) override;

      /**
       * @brief Requests the terminal to exit.
       */
      void RequestExit();

      /**
       * @brief Returns whether an exit has been requested.
       * @return `true` if an exit has been requested; `false` otherwise.
       */
      bool IsExitRequested() const { return _exitRequested; }

      /**
       * @brief Sets the internal padding of this terminal in pixels.
       * @param padding Padding in pixels.
       *
       * Text is inset from all edges by this amount; the padding area is filled
       * with the background color.
       */
      void SetPadding(UInt16 padding);

      /**
       * @brief Sets the foreground (text) color.
       * @param color 32-bit ARGB color value.
       */
      void SetForegroundColor(UInt32 color) { _foregroundColor = color; }

      /**
       * @brief Sets the background color.
       * @param color 32-bit ARGB color value.
       */
      void SetBackgroundColor(UInt32 color) { _backgroundColor = color; }

      /**
       * @brief Sets the cursor width in pixels.
       * @param width Cursor width (e.g. `2` for a thin bar, `8` for a block).
       */
      void SetCursorWidth(UInt8 width) { _cursorWidth = width; }

      /**
       * @brief Sets the extra spacing between lines in pixels.
       * @param spacing Line spacing in pixels.
       */
      void SetLineSpacing(UInt8 spacing) { _lineSpacing = spacing; }

      /**
       * @brief Resizes this terminal to fit new content area dimensions.
       * @param newWidth New content area width in pixels.
       * @param newHeight New content area height in pixels.
       *
       * Adjusts visible rows without reallocating the buffer. Content is
       * preserved since the buffer is allocated at max width.
       */
      void Resize(UInt16 newWidth, UInt16 newHeight);

      /**
       * @brief Re-renders this terminal's visible viewport to the owning
       *        @ref Canvas instance.
       */
      void Draw() override;

      /**
       * @brief Returns whether this terminal accepts keyboard input.
       * @return Always `true`.
       */
      bool AcceptsKeyboard() const override { return true; }

      /**
       * @brief Sends this terminal's accumulated dirty region to the display
       *        via the invalidate callback.
       *
       * Call after a batch of @ref Console::Write calls that occur outside
       * the event-driven methods (@ref Console::ProcessKeyEvent,
       * @ref Console::ScrollBy, @ref Console::Resize), which flush
       * automatically.
       */
      void Flush();

      /**
       * @brief Scrolls this terminal's viewport by the given number of lines.
       * @param lines Number of lines to scroll.
       *
       * Positive values scroll up (toward earlier output), negative
       * values scroll down (toward the cursor). Clamped to valid range.
       */
      void ScrollBy(Int16 lines);

      /**
       * @brief Resets this terminal's scroll offset so the cursor is visible at
       *        the bottom of the viewport.
       */
      void ScrollToBottom();

      /**
       * @brief Returns this terminal's total line count.
       * @return The number of lines with content in the buffer.
       */
      UInt16 GetTotalLines() const { return _lineCount; }

      /**
       * @brief Returns this terminal's visible column count.
       * @return The number of visible columns.
       */
      UInt16 GetColumns() const { return _columns; }

      /**
       * @brief Returns this terminal's visible row count.
       * @return The number of visible rows.
       */
      UInt16 GetVisibleRows() const { return _rows; }

      /**
       * @brief Returns the active font's maximum glyph width in pixels.
       * @return Glyph cell width in pixels.
       */
      UInt8 GetGlyphWidth() const { return _canvas.GetPainter().GetFont().Width; }

      /**
       * @brief Returns the active font's glyph height in pixels.
       * @return Glyph cell height in pixels.
       */
      UInt8 GetGlyphHeight() const { return _canvas.GetPainter().GetFont().Height; }

      /**
       * @brief Returns the row height (glyph height + line spacing).
       * @return Row stride in pixels.
       */
      UInt16 GetRowHeight() const {
        return static_cast<UInt16>(GetGlyphHeight() + _lineSpacing);
      }

    protected:
      /**
       * @brief Hook called after executing a command.
       * @param command The command line string.
       */
      virtual void AfterExecute(const char* command);

    private:
      /**
       * @brief Maximum columns (derived from surface stride at
       *        construction). The buffer row width is fixed at this value.
       */
      UInt16 _maxColumns;

      /**
       * @brief Physical index of the oldest row in the ring buffer.
       *
       * Logical row `i` maps to physical index
       * `(_ringHead + i) % MaxBufferRows`. Advancing the head discards
       * the oldest row in O(1) without shifting.
       */
      UInt16 _ringHead = 0;

      /**
       * @brief Current visible columns (active content width / glyph width).
       */
      UInt16 _columns;

      /**
       * @brief Current visible rows (active content height / row height).
       */
      UInt16 _rows;

      /**
       * @brief Cursor column (`0`-based).
       */
      UInt16 _cursorColumn = 0;

      /**
       * @brief Cursor row in absolute buffer coordinates (`0`-based).
       */
      UInt16 _cursorRow = 0;

      /**
       * @brief Total lines that have content in the buffer.
       *
       * Range is `[1, MaxBufferRows]`.
       */
      UInt16 _lineCount = 0;

      /**
       * @brief Length of each buffer row (number of chars written to that row).
       *
       * Each buffer row is a logical line; `_rowLength[r]` determines how many
       * visual rows it occupies when reflowed at the current pixel width.
       */
      UInt16 _rowLength[MaxBufferRows] = {};

      /**
       * @brief Cached pixel width of each buffer row's content.
       *
       * Updated incrementally on @ref _put and backspace. Used for
       * fast wrap-point detection without re-walking the entire row.
       */
      UInt16 _rowPixelWidth[MaxBufferRows] = {};

      /**
       * @brief Usable pixel width for text content (surface width minus
       *        padding on both sides).
       */
      UInt16 _usablePixelWidth = 0;

      /**
       * @brief Fixed scrollback buffer with size
       *        `MaxBufferRows * _maxColumns.`
       *
       * Never reallocated after construction.
       */
      char* _buffer;

      /**
       * @brief Internal padding in pixels from all edges.
       */
      UInt16 _padding = 0;

      /**
       * @brief Extra pixels between lines.
       */
      UInt8 _lineSpacing = DefaultLineSpacing;

      /**
       * @brief Cursor width in pixels.
       */
      UInt8 _cursorWidth = DefaultCursorWidth;

      /**
       * @brief Foreground color (glyph color).
       */
      UInt32 _foregroundColor = DefaultForeground;

      /**
       * @brief Background color.
       */
      UInt32 _backgroundColor = DefaultBackground;

      /**
       * @brief The prompt string displayed before each input line.
       */
      char _prompt[MaxPromptLength] = {};

      /**
       * @brief Current input line buffer.
       */
      char _lineBuffer[MaxLineLength] = {};

      /**
       * @brief Current write position within @ref _lineBuffer.
       */
      Size _lineIndex = 0;

      /**
       * @brief Registered command table.
       */
      ConsoleCommandEntry _commands[MaxCommands] = {};

      /**
       * @brief Number of registered commands.
       */
      Size _commandCount = 0;

      /**
       * @brief Fallback handler for unrecognized commands.
       */
      ConsoleCommandHandler _defaultHandler = nullptr;

      /**
       * @brief Callback invoked when the dirty region needs flushing.
       */
      ConsoleInvalidateCallback _invalidateCallback = nullptr;

      /**
       * @brief Opaque context pointer passed to @ref _invalidateCallback.
       */
      void* _invalidateContext = nullptr;

      /**
       * @brief Whether an exit has been requested.
       */
      bool _exitRequested = false;

      /**
       * @brief Manual scroll offset in lines above the auto-scroll
       *        position. `0` = following cursor (default).
       */
      Int16 _scrollOffset = 0;

      /**
       * @brief Accumulated dirty region for the current render cycle.
       */
      Geometry2D::Rectangle _dirtyRectangle;

      /**
       * @brief Dirty region from the previous render cycle, used for
       *        shift-based invalidation.
       */
      Geometry2D::Rectangle _lastDirtyRectangle;

      /**
       * @brief Whether the next draw should repaint the entire viewport.
       */
      bool _doingFullRedraw = true;

      /**
       * @brief Whether a scroll operation is pending for the next draw.
       */
      bool _isScrollPending = false;

      /**
       * @brief Previous viewport top row, used to detect scroll direction.
       */
      UInt16 _previousViewportTop = 0;

      /**
       * @brief Pixel shift hint passed to the invalidate callback.
       *        Positive = content shifted up.
       */
      Int16 _shiftDeltaPixels = 0;

      /**
       * @brief Maps a logical row index (0 = oldest) to its physical
       *        index in the ring buffer.
       * @param logicalRow Zero-based logical row index.
       * @return Physical buffer row index.
       */
      UInt16 _physicalRow(UInt16 logicalRow) const {
        return static_cast<UInt16>(
          (_ringHead + logicalRow) % MaxBufferRows
        );
      }

      /**
       * @brief Returns the number of visual rows a buffer row occupies
       *        when wrapped at the current pixel width.
       * @param bufferRow The buffer row index.
       * @return The number of visual segments (always >= `1`).
       */
      UInt16 _getSegmentCount(UInt16 bufferRow) const;

      /**
       * @brief Returns the character offset and count for a given visual
       *        segment of a buffer row.
       * @param bufferRow The buffer row index.
       * @param segment The `0`-based segment index within that row.
       * @param outStart Receives the character start offset.
       * @param outCount Receives the character count for this segment.
       */
      void _getSegmentRange(
        UInt16 bufferRow,
        UInt16 segment,
        UInt16& outStart,
        UInt16& outCount
      ) const;

      /**
       * @brief Returns the pixel X offset of the cursor within its
       *        current visual segment.
       * @return Pixel X offset from the left edge of the segment.
       */
      UInt16 _getCursorPixelX() const;

      /**
       * @brief Marks a single visual row (in viewport space) as dirty.
       * @param screenRow The `0`-based viewport row index.
       */
      void _markVisualRowDirty(UInt16 screenRow);

      /**
       * @brief Marks the visual row containing the cursor as dirty.
       */
      void _markCursorRowDirty();

      /**
       * @brief Marks the single glyph cell at the cursor position as dirty.
       */
      void _markCursorCellDirty();

      /**
       * @brief Returns the visual row index of the first visible line,
       *        accounting for reflow and the manual scroll offset.
       * @return Visual row index of the viewport top.
       */
      UInt16 _getViewportTop() const;

      /**
       * @brief Returns the visual row index of the cursor, counting all
       *        visual rows produced by reflowing buffer rows.
       * @return Absolute visual row index of the cursor.
       */
      UInt16 _getCursorVisualRow() const;

      /**
       * @brief Writes a single character at the cursor and advances it.
       * @param character The character to write.
       */
      void _put(char character);

      /**
       * @brief Advances the cursor to the next line, auto-scrolling the
       *        viewport if the cursor moves past the bottom.
       */
      void _newLine();

      /**
       * @brief Prints the prompt to the terminal.
       */
      void _printPrompt();

      /**
       * @brief Submits the current line buffer for execution.
       */
      void _submitLine();

      /**
       * @brief Fires the invalidate callback with the accumulated dirty
       *        region, then resets it. No-op when nothing is dirty.
       */
      void _flush();

      /**
       * @brief Tokenises a line into arguments.
       * @param line Mutable line buffer (modified in place with null
       *             terminators).
       * @param arguments Output array of argument pointers.
       * @param maxArgumentCount Maximum argument count.
       * @return Number of arguments parsed.
       */
      Size _parseLine(
        char* line,
        const char** arguments,
        Size maxArgumentCount
      );
  };
}
