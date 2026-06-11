/**
 * @file Components/Console.cpp
 * @brief Implements @ref Quantum::UI::Console.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "ComponentTypes.hpp"

namespace Quantum::Components {
  Console::Console(Canvas& surface) : Element(surface) {
    // buffer stride is fixed at MaxColumns regardless of initial window width,
    // so the scrollback buffer never needs reallocation on resize
    _maxColumns = MaxColumns;

    // visible area from active surface dimensions (accounting for padding)
    UInt16 usableWidth = surface.GetWidth() > 2 * _padding
      ? static_cast<UInt16>(surface.GetWidth() - 2 * _padding)
      : 0;
    UInt16 usableHeight = surface.GetHeight() > 2 * _padding
      ? static_cast<UInt16>(surface.GetHeight() - 2 * _padding)
      : 0;

    _usablePixelWidth = usableWidth;

    // _cols is now an approximate column count used for MaxColumns checks
    UInt8 avgAdvance = GetGlyphWidth();

    _columns = avgAdvance > 0
      ? static_cast<UInt16>(usableWidth / avgAdvance)
      : 1;
    _rows = static_cast<UInt16>(usableHeight / GetRowHeight());

    if (_columns == 0) _columns = 1;
    if (_rows == 0) _rows = 1;
    if (_columns > _maxColumns) _columns = _maxColumns;

    // allocate fixed scrollback buffer (never reallocated)
    Size bufferSize = static_cast<Size>(_maxColumns) * MaxBufferRows;
    UIntPtr buffer = AllocateBlock(bufferSize);

    _buffer = reinterpret_cast<char*>(buffer);

    Byte::Fill(_buffer, ' ', bufferSize);

    for (UInt16 r = 0; r < MaxBufferRows; r++) {
      _rowLength[r] = 0;
      _rowPixelWidth[r] = 0;
    }

    _lineCount = 1;
    _cursorRow = 0;
    _cursorColumn = 0;

    surface.GetPainter().Clear(_backgroundColor);
  }

  Console::~Console() {
    if (_buffer) FreeBlock(_buffer);
  }

  void Console::Write(const char* text) {
    if (!text) return;

    for (Size i = 0; text[i]; ++i) {
      char ch = text[i];

      if (ch == '\n') {
        _newLine();
      } else if (ch == '\t') {
        // advance to the next tab stop (fixed pixel interval)
        UInt16 currentPx = _rowPixelWidth[_cursorRow];
        UInt16 nextStop = static_cast<UInt16>(
          ((currentPx / TabStopPixels) + 1) * TabStopPixels
        );
        UInt16 spacesNeeded = 0;
        const auto& tabFont = _canvas.GetPainter().GetFont();
        UInt8 spaceAdv = tabFont.GetAdvance(static_cast<UInt8>(' '));

        if (spaceAdv > 0) {
          spacesNeeded = static_cast<UInt16>(
            (nextStop - currentPx + spaceAdv - 1) / spaceAdv
          );
        }

        for (UInt16 s = 0; s < spacesNeeded; ++s) _put(' ');
      } else if (ch == '\b') {
        if (_cursorColumn > 0) {
          if (!_doingFullRedraw && !_isScrollPending) _markCursorCellDirty();

          UInt16 oldSegs = _getSegmentCount(_cursorRow);

          // subtract the advance of the character being deleted
          const auto& font = _canvas.GetPainter().GetFont();
          UInt8 deletedAdv = font.GetAdvance(static_cast<UInt8>(
            _buffer[
                static_cast<Size>(_cursorRow) * _maxColumns
              + (_cursorColumn - 1)
            ]
          ));

          _cursorColumn--;

          _buffer[
              static_cast<Size>(_cursorRow) * _maxColumns
            + _cursorColumn
          ] = ' ';

          // shrink row length if we erased the last character
          if (_cursorColumn + 1 == _rowLength[_cursorRow]) {
            _rowLength[_cursorRow] = _cursorColumn;
          }

          // update the cached pixel width
          if (_rowPixelWidth[_cursorRow] >= deletedAdv) {
            _rowPixelWidth[_cursorRow] = static_cast<UInt16>(
              _rowPixelWidth[_cursorRow] - deletedAdv
            );
          } else {
            _rowPixelWidth[_cursorRow] = 0;
          }

          if (!_doingFullRedraw && !_isScrollPending) {
            UInt16 newSegs = _getSegmentCount(_cursorRow);

            if (newSegs != oldSegs) {
              if (_getViewportTop() > _previousViewportTop) {
                _isScrollPending = true;
              } else {
                _doingFullRedraw = true;
              }
            } else {
              _markCursorCellDirty();
            }
          }
        }
      } else if (ch == '\r') {
        _cursorColumn = 0;
        _doingFullRedraw = true;
      } else {
        _put(ch);
      }
    }

    Draw();
  }

  void Console::WriteLine(const char* text) {
    Write(text);
    Write("\n");
  }

  void Console::TabTo(UInt16 pixelColumn) {
    if (_rowPixelWidth[_cursorRow] >= pixelColumn) {
      // already past the target, emit a single space so columns
      // never visually merge
      _put(' ');

      return;
    }

    // need 3 buffer slots: sentinel + low byte + high byte
    if (_cursorColumn + 3 > _maxColumns) return;

    Size base = static_cast<Size>(_cursorRow) * _maxColumns + _cursorColumn;

    _buffer[base] = TabSentinel;
    _buffer[base + 1] = static_cast<char>(pixelColumn & 0xFF);
    _buffer[base + 2] = static_cast<char>((pixelColumn >> 8) & 0xFF);

    _cursorColumn = static_cast<UInt16>(_cursorColumn + 3);

    if (_cursorColumn > _rowLength[_cursorRow]) {
      _rowLength[_cursorRow] = _cursorColumn;
    }

    _rowPixelWidth[_cursorRow] = pixelColumn;

    if (!_doingFullRedraw && !_isScrollPending) {
      _markCursorRowDirty();
    }
  }

  void Console::Clear() {
    Size bufferSize = static_cast<Size>(_maxColumns) * MaxBufferRows;

    Byte::Fill(_buffer, ' ', bufferSize);

    for (UInt16 r = 0; r < MaxBufferRows; r++) {
      _rowLength[r] = 0;
      _rowPixelWidth[r] = 0;
    }

    _lineCount = 1;
    _cursorColumn = 0;
    _cursorRow = 0;
    _ringHead = 0;
    _doingFullRedraw = true;

    _canvas.GetPainter().Clear(_backgroundColor);
  }

  void Console::SetPrompt(const char* prompt) {
    CString::Copy(prompt, _prompt, MaxPromptLength);
  }

  void Console::RegisterCommand(
    const char* name,
    ConsoleCommandHandler handler
  ) {
    if (_commandCount >= MaxCommands) return;

    _commands[_commandCount].name = name;
    _commands[_commandCount].handler = handler;
    _commandCount++;
  }

  void Console::SetDefaultHandler(ConsoleCommandHandler handler) {
    _defaultHandler = handler;
  }

  void Console::SetInvalidateCallback(
    ConsoleInvalidateCallback callback,
    void* context
  ) {
    _invalidateCallback = callback;
    _invalidateContext = context;
  }

  void Console::Execute(const char* command) {
    if (CString::Length(command) == 0) return;

    char workBuffer[MaxLineLength];

    CString::Copy(command, workBuffer, MaxLineLength);

    const char* arguments[MaxArguments] = {};
    Size argumentCount = _parseLine(workBuffer, arguments, MaxArguments);

    if (argumentCount == 0) return;

    for (Size i = 0; i < _commandCount; i++) {
      if (CString::Equals(arguments[0], _commands[i].name)) {
        ConsoleCommandContext context {
          static_cast<UInt32>(argumentCount),
          arguments,
          command
        };

        _commands[i].handler(*this, context);

        AfterExecute(command);

        return;
      }
    }

    if (_defaultHandler) {
      ConsoleCommandContext context {
        static_cast<UInt32>(argumentCount),
        arguments,
        command
      };

      _defaultHandler(*this, context);
    } else {
      char msg[320];

      CString::Format(msg, sizeof(msg), "Unknown command: %s\n", arguments[0]);

      Write(msg);
    }

    AfterExecute(command);
  }

  bool Console::ProcessKeyEvent(const InputEvent& event) {
    if (event.Type != InputEventType::KeyDown) return false;

    // Ctrl+C: cancel current line
    if (
      Enum::HasAnyFlag(
        static_cast<KeyModifiers>(event.Modifiers),
        KeyModifiers::Ctrl
      ) && event.Character == 'c'
    ) {
      Write("^C\n");

      _lineIndex = 0;
      _lineBuffer[0] = '\0';

      _printPrompt();
      _flush();

      return true;
    }

    if (event.Character == '\n') {
      Write("\n");
      _flush();

      _lineBuffer[_lineIndex] = '\0';

      Execute(_lineBuffer);

      _lineIndex = 0;
      _lineBuffer[0] = '\0';

      if (!_exitRequested) {
        _printPrompt();
      }
    } else if (event.Character == '\b') {
      if (_lineIndex > 0) {
        _lineIndex--;

        Write("\b \b");
      }
    } else if (
      event.Character != 0 &&
      event.Character != '\t' &&
      _lineIndex < MaxLineLength - 1
    ) {
      _lineBuffer[_lineIndex++] = event.Character;

      char echo[2] = { event.Character, '\0' };

      Write(echo);
    }

    _flush();

    return true;
  }

  void Console::RequestExit() {
    _exitRequested = true;
  }

  void Console::SetPadding(UInt16 padding) {
    _padding = padding;

    UInt16 w = _canvas.GetWidth();
    UInt16 h = _canvas.GetHeight();
    UInt16 usableW = w > 2 * _padding
      ? static_cast<UInt16>(w - 2 * _padding)
      : 0;
    UInt16 usableH = h > 2 * _padding
      ? static_cast<UInt16>(h - 2 * _padding)
      : 0;

    _usablePixelWidth = usableW;

    UInt8 avgAdvance = GetGlyphWidth();

    _columns = avgAdvance > 0
      ? static_cast<UInt16>(usableW / avgAdvance)
      : 1;
    _rows = static_cast<UInt16>(usableH / GetRowHeight());

    if (_columns == 0) _columns = 1;
    if (_rows == 0) _rows = 1;
    if (_columns > _maxColumns) _columns = _maxColumns;
  }

  void Console::Resize(UInt16 newWidth, UInt16 newHeight) {
    UInt16 usableW = newWidth > 2 * _padding
      ? static_cast<UInt16>(newWidth - 2 * _padding)
      : 0;
    UInt16 usableH = newHeight > 2 * _padding
      ? static_cast<UInt16>(newHeight - 2 * _padding)
      : 0;

    _usablePixelWidth = usableW;

    UInt8 avgAdvance = GetGlyphWidth();
    UInt16 newCols = avgAdvance > 0
      ? static_cast<UInt16>(usableW / avgAdvance)
      : 1;
    UInt16 newRows = static_cast<UInt16>(usableH / GetRowHeight());

    if (newCols == 0) newCols = 1;
    if (newRows == 0) newRows = 1;
    if (newCols > _maxColumns) newCols = _maxColumns;

    _columns = newCols;
    _rows = newRows;

    // always update surface dimensions and redraw - the underlying pixel
    // buffer changes on every resize (a new shared buffer is allocated),
    // so Draw() must be called even when the character-grid size is
    // unchanged
    // skipping it leaves the staged buffer filled with ContentColor and the
    // compositor blits black, erasing all content
    _canvas.SetDimensions(newWidth, newHeight);

    _doingFullRedraw = true;

    Draw();
    _flush();
  }

  void Console::Draw() {
    if (!_canvas.IsValid()) return;

    // handle scroll-blit: when the viewport scrolled by a small amount,
    // shift the surface pixels up and only dirty the new bottom rows
    if (_isScrollPending) {
      _shiftDeltaPixels = 0;
      UInt16 newTop = _getViewportTop();

      // the scroll-blit optimization is only safe when no dirty cells
      // have been accumulated in the current Draw() cycle; if _dirtyRect
      // is non-empty, chars were put before a '\n' in the same Write()
      // call, their dirty marks would be lost by overwriting _dirtyRect,
      // so bail to a full redraw instead
      if (_previousViewportTop < newTop && _dirtyRectangle.IsEmpty()) {
        UInt16 delta = static_cast<UInt16>(newTop - _previousViewportTop);

        if (delta <= 3 && _scrollOffset == 0) {
          _canvas.GetPainter().ScrollUp(
            static_cast<UInt16>(delta * GetRowHeight()), _backgroundColor
          );

          // dirty the new bottom row(s) plus one extra row above to erase
          // the old cursor block that ScrollUp shifted out of the new region
          UInt16 dirtyY = static_cast<UInt16>(
            (
              _rows > delta
                ? _rows - delta - 1
                : 0
            ) * GetRowHeight() + _padding
          );

          _dirtyRectangle = Rectangle(
            0,
            static_cast<Int16>(dirtyY),
            _canvas.GetWidth(),
            static_cast<UInt16>(_canvas.GetHeight() - dirtyY)
          );
          _shiftDeltaPixels = static_cast<Int16>(delta * GetRowHeight());
          _isScrollPending = false;
          _doingFullRedraw = false;
        } else {
          _isScrollPending = false;
          _doingFullRedraw = true;
        }
      } else {
        _isScrollPending = false;
        _doingFullRedraw = true;
      }
    }

    if (_doingFullRedraw) {
      _shiftDeltaPixels = 0;
      _dirtyRectangle = Rectangle(
        0,
        0,
        _canvas.GetWidth(),
        _canvas.GetHeight()
      );
      _doingFullRedraw = false;

      if (_padding > 0) {
        _canvas.GetPainter().Clear(_backgroundColor);
      }
    }

    // when a previous cycle accumulated dirty state and the current
    // cycle produced a shift delta, the compositor cannot use the
    // shift optimization safely, expand to full surface and clear
    if (!_lastDirtyRectangle.IsEmpty() && _shiftDeltaPixels != 0) {
      _shiftDeltaPixels = 0;
      _lastDirtyRectangle = Rectangle(
        0,
        0,
        _canvas.GetWidth(),
        _canvas.GetHeight()
      );
    }

    // accumulate into the output rect (survives multiple Draw() calls
    // within a single invalidation cycle)
    if (_lastDirtyRectangle.IsEmpty()) {
      _lastDirtyRectangle = _dirtyRectangle;
    } else if (!_dirtyRectangle.IsEmpty()) {
      _lastDirtyRectangle = _lastDirtyRectangle.Union(_dirtyRectangle);
    }

    if (_dirtyRectangle.IsEmpty()) {
      _previousViewportTop = _getViewportTop();

      return;
    }

    // compute the range of visual rows that need redrawing
    Int16 adjY = static_cast<Int16>(
      _dirtyRectangle.Origin.Y > _padding
        ? _dirtyRectangle.Origin.Y - _padding
        : 0
    );
    Int16 adjBottom = static_cast<Int16>(
      _dirtyRectangle.GetBottom() > _padding
        ? _dirtyRectangle.GetBottom() - _padding
        : 0
    );

    UInt16 dirtyTopRow = static_cast<UInt16>(adjY / GetRowHeight());
    UInt16 dirtyBottomRow = static_cast<UInt16>(
      (adjBottom + GetRowHeight() - 1) / GetRowHeight()
    );

    if (dirtyBottomRow > _rows) dirtyBottomRow = _rows;

    UInt16 startVR = _getViewportTop();
    UInt16 renderedVR = 0;
    UInt16 skippedVR = 0;

    for (UInt16 i = 0; i < _lineCount && renderedVR < _rows; i++) {
      UInt16 br = _physicalRow(i);
      UInt16 segs = _getSegmentCount(br);

      for (UInt16 seg = 0; seg < segs && renderedVR < _rows; seg++) {
        if (skippedVR < startVR) {
          skippedVR++;

          continue;
        }

        // only redraw visual rows within the dirty range
        if (renderedVR >= dirtyTopRow && renderedVR < dirtyBottomRow) {
          UInt16 charStart = 0;
          UInt16 charCount = 0;

          _getSegmentRange(br, seg, charStart, charCount);

          UInt16 py = static_cast<UInt16>(
            renderedVR * GetRowHeight() + _padding
          );

          // clear the entire visual row background first
          _canvas.GetPainter().FillRectangle(
            Rectangle(
              static_cast<Int16>(_padding),
              static_cast<Int16>(py),
              _usablePixelWidth,
              GetRowHeight()
            ),
            _backgroundColor
          );

          if (charCount > 0) {
            Size rowBase = static_cast<Size>(br) * _maxColumns;
            UInt16 cx = _padding;
            UInt16 chunkStart = charStart;

            for (UInt16 c = charStart; c < charStart + charCount; ++c) {
              if (
                _buffer[rowBase + c] == TabSentinel &&
                c + 2 < charStart + charCount
              ) {
                // draw the text chunk before this sentinel
                if (c > chunkStart) {
                  char saved = _buffer[rowBase + c];

                  _buffer[rowBase + c] = '\0';

                  _canvas.GetPainter().DrawText(
                    static_cast<Int16>(cx),
                    static_cast<Int16>(py),
                    &_buffer[rowBase + chunkStart],
                    _foregroundColor,
                    _backgroundColor
                  );

                  _buffer[rowBase + c] = saved;
                }

                // jump X cursor to the encoded pixel column
                UInt16 target = static_cast<UInt16>(
                    static_cast<UInt8>(_buffer[rowBase + c + 1])
                  | (static_cast<UInt8>(_buffer[rowBase + c + 2]) << 8)
                );

                cx = static_cast<UInt16>(_padding + target);
                c = static_cast<UInt16>(c + 2);
                chunkStart = static_cast<UInt16>(c + 1);

                continue;
              }
            }

            // draw the final chunk after the last sentinel (or the
            // whole line if there were no sentinels)
            UInt16 endPos = static_cast<UInt16>(charStart + charCount);

            if (chunkStart < endPos) {
              char saved = _buffer[rowBase + endPos];

              _buffer[rowBase + endPos] = '\0';

              _canvas.GetPainter().DrawText(
                static_cast<Int16>(cx),
                static_cast<Int16>(py),
                &_buffer[rowBase + chunkStart],
                _foregroundColor,
                _backgroundColor
              );

              _buffer[rowBase + endPos] = saved;
            }
          }
        }

        renderedVR++;
      }
    }

    // clear the area below the last rendered row (if within dirty range)
    if (renderedVR < _rows && renderedVR < dirtyBottomRow) {
      UInt16 clearY = static_cast<UInt16>(
        renderedVR * GetRowHeight() + _padding
      );

      if (clearY < static_cast<UInt16>(_dirtyRectangle.GetBottom())) {
        _canvas.GetPainter().FillRectangle(
          Rectangle(
            static_cast<Int16>(_padding),
            static_cast<Int16>(clearY),
            static_cast<UInt16>(_canvas.GetWidth() - 2 * _padding),
            static_cast<UInt16>(_canvas.GetHeight() - clearY)
          ),
          _backgroundColor
        );
      }
    }

    // draw block cursor (if its row is within the dirty range)
    UInt16 cursorVR = _getCursorVisualRow();

    if (
      cursorVR >= startVR &&
      cursorVR < static_cast<UInt16>(startVR + _rows)
    ) {
      UInt16 screenRow = static_cast<UInt16>(cursorVR - startVR);

      if (screenRow >= dirtyTopRow && screenRow < dirtyBottomRow) {
        UInt16 cursorPx = _getCursorPixelX();
        UInt8 cursorWidth = _cursorWidth;

        const auto& cursorFont = _canvas.GetPainter().GetFont();
        UInt8 cursorTop = (cursorFont.InkTop > 1)
          ? static_cast<UInt8>(cursorFont.InkTop - 1)
          : 0;
        UInt8 cursorBottom = static_cast<UInt8>(
          cursorFont.Ascent + 2 <= cursorFont.Height
            ? cursorFont.Ascent + 2
            : cursorFont.Height
        );
        UInt8 cursorHeight = static_cast<UInt8>(cursorBottom - cursorTop);

        _canvas.GetPainter().FillRectangle(
          Rectangle(
            static_cast<Int16>(cursorPx + _padding),
            static_cast<Int16>(
              screenRow * GetRowHeight() + _padding + cursorTop
            ),
            cursorWidth,
            cursorHeight
          ),
          _foregroundColor
        );
      }
    }

    // reset for the next mutation cycle
    _dirtyRectangle = Rectangle();
    _previousViewportTop = _getViewportTop();
  }

  void Console::Flush() { _flush(); }

  void Console::ScrollBy(Int16 lines) {
    Int16 newOffset = static_cast<Int16>(_scrollOffset + lines);

    // maximum scroll: can scroll up by at most the auto-scroll top (visual
    // rows)
    UInt16 cursorVR = _getCursorVisualRow();
    UInt16 autoTop = cursorVR >= _rows
      ? static_cast<UInt16>(cursorVR - _rows + 1)
      : 0;

    if (newOffset < 0) {
      newOffset = 0;
    }

    if (newOffset > static_cast<Int16>(autoTop)) {
      newOffset = static_cast<Int16>(autoTop);
    }

    if (newOffset == _scrollOffset) {
      return;
    }

    UInt16 oldTop = _getViewportTop();

    _scrollOffset = newOffset;

    UInt16 newTop = _getViewportTop();

    if (newTop == oldTop) {
      _doingFullRedraw = true;

      Draw();
      _flush();

      return;
    }

    Int16 deltaVR = static_cast<Int16>(
      static_cast<Int16>(newTop) - static_cast<Int16>(oldTop)
    );
    Int16 absDelta = deltaVR < 0 ? static_cast<Int16>(-deltaVR) : deltaVR;

    if (absDelta <= 3) {
      UInt16 pixelShift = static_cast<UInt16>(absDelta * GetRowHeight());

      if (deltaVR > 0) {
        _canvas.GetPainter().ScrollUp(pixelShift, _backgroundColor);

        _shiftDeltaPixels = static_cast<Int16>(pixelShift);

        UInt16 dirtyStartRow = (_rows > static_cast<UInt16>(absDelta))
          ? static_cast<UInt16>(_rows - absDelta)
          : 0;
        UInt16 dirtyY = static_cast<UInt16>(
          dirtyStartRow * GetRowHeight() + _padding
        );

        _dirtyRectangle = Rectangle(
          0,
          static_cast<Int16>(dirtyY),
          _canvas.GetWidth(),
          static_cast<UInt16>(_canvas.GetHeight() - dirtyY)
        );
      } else {
        _canvas.GetPainter().ScrollDown(pixelShift, _backgroundColor);

        _shiftDeltaPixels = static_cast<Int16>(-pixelShift);

        _dirtyRectangle = Rectangle(
          0,
          0,
          _canvas.GetWidth(),
          static_cast<UInt16>(pixelShift + _padding)
        );
      }
    } else {
      _doingFullRedraw = true;
    }

    Draw();
    _flush();
  }

  void Console::ScrollToBottom() {
    if (_scrollOffset == 0) return;

    _scrollOffset = 0;
    _doingFullRedraw = true;

    Draw();
  }

  // --------------------------------------------------------------------------
  // Protected
  // --------------------------------------------------------------------------

  void Console::AfterExecute(const char* /*command*/) {}

  // --------------------------------------------------------------------------
  // Private
  // --------------------------------------------------------------------------

  UInt16 Console::_getSegmentCount(UInt16 bufferRow) const {
    UInt16 len = _rowLength[bufferRow];

    if (len == 0) return 1;

    const auto& font = _canvas.GetPainter().GetFont();
    Size rowBase = static_cast<Size>(bufferRow) * _maxColumns;
    UInt16 segs = 1;
    UInt16 lineWidth = 0;

    for (UInt16 c = 0; c < len; ++c) {
      if (_buffer[rowBase + c] == TabSentinel && c + 2 < len) {
        UInt16 target = static_cast<UInt16>(
            static_cast<UInt8>(_buffer[rowBase + c + 1])
          | (static_cast<UInt8>(_buffer[rowBase + c + 2]) << 8)
        );

        lineWidth = target;
        c = static_cast<UInt16>(c + 2);

        continue;
      }

      UInt8 adv = font.GetAdvance(static_cast<UInt8>(
        _buffer[rowBase + c]
      ));

      if (lineWidth + adv > _usablePixelWidth && lineWidth > 0) {
        segs++;
        lineWidth = adv;
      } else {
        lineWidth += adv;
      }
    }

    return segs;
  }

  void Console::_getSegmentRange(
    UInt16 bufferRow,
    UInt16 segment,
    UInt16& outStart,
    UInt16& outCount
  ) const {
    UInt16 len = _rowLength[bufferRow];
    if (len == 0 || segment == 0) {
      outStart = 0;
    }

    if (len == 0) {
      outCount = 0;
      return;
    }

    const auto& font = _canvas.GetPainter().GetFont();
    Size rowBase = static_cast<Size>(bufferRow) * _maxColumns;
    UInt16 currentSeg = 0;
    UInt16 segStart = 0;
    UInt16 lineWidth = 0;

    for (UInt16 c = 0; c < len; ++c) {
      if (_buffer[rowBase + c] == TabSentinel && c + 2 < len) {
        UInt16 target = static_cast<UInt16>(
            static_cast<UInt8>(_buffer[rowBase + c + 1])
          | (static_cast<UInt8>(_buffer[rowBase + c + 2]) << 8)
        );

        lineWidth = target;
        c = static_cast<UInt16>(c + 2);

        continue;
      }

      UInt8 adv = font.GetAdvance(static_cast<UInt8>(
        _buffer[rowBase + c]
      ));

      if (lineWidth + adv > _usablePixelWidth && lineWidth > 0) {
        if (currentSeg == segment) {
          outStart = segStart;
          outCount = static_cast<UInt16>(c - segStart);
          return;
        }

        currentSeg++;
        segStart = c;
        lineWidth = adv;
      } else {
        lineWidth += adv;
      }
    }

    // last (or only) segment
    outStart = segStart;
    outCount = static_cast<UInt16>(len - segStart);
  }

  UInt16 Console::_getCursorPixelX() const {
    const auto& font = _canvas.GetPainter().GetFont();
    Size rowBase = static_cast<Size>(_cursorRow) * _maxColumns;
    UInt16 lineWidth = 0;
    UInt16 segStart = 0;

    // walk to find which segment the cursor is in
    for (UInt16 c = 0; c < _cursorColumn; ++c) {
      if (_buffer[rowBase + c] == TabSentinel && c + 2 < _cursorColumn) {
        UInt16 target = static_cast<UInt16>(
            static_cast<UInt8>(_buffer[rowBase + c + 1])
          | (static_cast<UInt8>(_buffer[rowBase + c + 2]) << 8)
        );

        lineWidth = target;
        c = static_cast<UInt16>(c + 2);

        continue;
      }

      UInt8 adv = font.GetAdvance(static_cast<UInt8>(
        _buffer[rowBase + c]
      ));

      if (lineWidth + adv > _usablePixelWidth && lineWidth > 0) {
        segStart = c;
        lineWidth = adv;
      } else {
        lineWidth += adv;
      }
    }

    // now compute pixel offset from segStart to _cursorColumn
    UInt16 px = 0;

    for (UInt16 c = segStart; c < _cursorColumn; ++c) {
      if (_buffer[rowBase + c] == TabSentinel && c + 2 < _cursorColumn) {
        px = static_cast<UInt16>(
            static_cast<UInt8>(_buffer[rowBase + c + 1])
          | (static_cast<UInt8>(_buffer[rowBase + c + 2]) << 8)
        );

        c = static_cast<UInt16>(c + 2);

        continue;
      }

      px = static_cast<UInt16>(px + font.GetAdvance(static_cast<UInt8>(
        _buffer[rowBase + c]
      )));
    }

    return px;
  }

  void Console::_markVisualRowDirty(UInt16 screenRow) {
    if (screenRow >= _rows) return;

    UInt16 py = static_cast<UInt16>(screenRow * GetRowHeight() + _padding);

    Rectangle rowRectangle(
      static_cast<Int16>(_padding),
      static_cast<Int16>(py),
      _usablePixelWidth,
      GetRowHeight()
    );

    if (_dirtyRectangle.IsEmpty()) {
      _dirtyRectangle = rowRectangle;
    } else {
      _dirtyRectangle = _dirtyRectangle.Union(rowRectangle);
    }
  }

  void Console::_markCursorRowDirty() {
    UInt16 startVR = _getViewportTop();
    UInt16 cursorVR = _getCursorVisualRow();

    if (
      cursorVR < startVR ||
      cursorVR >= static_cast<UInt16>(startVR + _rows)
    ) return;

    _markVisualRowDirty(static_cast<UInt16>(cursorVR - startVR));
  }

  void Console::_markCursorCellDirty() {
    // for proportional fonts, dirty the entire visual row containing the
    // cursor rather than a single fixed-width cell
    _markCursorRowDirty();
  }

  UInt16 Console::_getViewportTop() const {
    UInt16 cursorVR = _getCursorVisualRow();
    UInt16 autoTop = (cursorVR >= _rows)
      ? static_cast<UInt16>(cursorVR - _rows + 1)
      : 0;

    if (_scrollOffset > 0) {
      if (autoTop >= static_cast<UInt16>(_scrollOffset)) {
        return static_cast<UInt16>(autoTop - _scrollOffset);
      }

      return 0;
    }

    return autoTop;
  }

  UInt16 Console::_getCursorVisualRow() const {
    UInt16 vr = 0;

    // iterate over all logical rows before the cursor (the cursor is
    // always the last logical row, at index _lineCount - 1)
    for (UInt16 i = 0; i + 1 < _lineCount; i++) {
      vr += _getSegmentCount(_physicalRow(i));
    }

    // determine which segment the cursor is in within its row
    const auto& font = _canvas.GetPainter().GetFont();
    Size rowBase = static_cast<Size>(_cursorRow) * _maxColumns;
    UInt16 lineWidth = 0;

    for (UInt16 c = 0; c < _cursorColumn; ++c) {
      if (_buffer[rowBase + c] == TabSentinel && c + 2 < _cursorColumn) {
        UInt16 target = static_cast<UInt16>(
            static_cast<UInt8>(_buffer[rowBase + c + 1])
          | (static_cast<UInt8>(_buffer[rowBase + c + 2]) << 8)
        );

        lineWidth = target;
        c = static_cast<UInt16>(c + 2);

        continue;
      }

      UInt8 adv = font.GetAdvance(static_cast<UInt8>(
        _buffer[rowBase + c]
      ));

      if (lineWidth + adv > _usablePixelWidth && lineWidth > 0) {
        vr++;
        lineWidth = adv;
      } else {
        lineWidth += adv;
      }
    }

    return vr;
  }

  void Console::_put(char ch) {
    if (_scrollOffset != 0) {
      _scrollOffset = 0;
      _doingFullRedraw = true;
    }

    // hard wrap at buffer boundary (requires >MaxColumns chars on one line)
    if (_cursorColumn >= _maxColumns) _newLine();

    const auto& font = _canvas.GetPainter().GetFont();
    UInt8 adv = font.GetAdvance(static_cast<UInt8>(ch));

    UInt16 oldSegs = _getSegmentCount(_cursorRow);

    // mark the visual row where the old cursor sits (will be overwritten)
    if (!_doingFullRedraw && !_isScrollPending) {
      _markCursorCellDirty();
    }

    _buffer[static_cast<Size>(_cursorRow) * _maxColumns + _cursorColumn] = ch;

    if (_cursorColumn + 1 > _rowLength[_cursorRow]) {
      _rowLength[_cursorRow] = static_cast<UInt16>(_cursorColumn + 1);
    }

    _cursorColumn++;
    _rowPixelWidth[_cursorRow] = static_cast<UInt16>(
      _rowPixelWidth[_cursorRow] + adv
    );

    UInt16 newSegs = _getSegmentCount(_cursorRow);

    if (!_doingFullRedraw && !_isScrollPending) {
      if (newSegs != oldSegs) {
        if (_getViewportTop() > _previousViewportTop) {
          _isScrollPending = true;
        } else {
          _doingFullRedraw = true;
        }
      } else {
        _markCursorCellDirty();
      }
    }
  }

  void Console::_newLine() {
    if (_scrollOffset != 0) {
      _scrollOffset = 0;
      _doingFullRedraw = true;
    }

    UInt16 oldCursorVR = 0;

    if (!_doingFullRedraw && !_isScrollPending) {
      oldCursorVR = _getCursorVisualRow();
    }

    _cursorColumn = 0;

    bool bufferWrapped = (_lineCount == MaxBufferRows);

    // advance cursor to the next physical row in the ring
    _cursorRow = static_cast<UInt16>((_cursorRow + 1) % MaxBufferRows);

    if (_lineCount < MaxBufferRows) {
      _lineCount++;
    } else {
      // buffer full: discard the oldest row by advancing the head
      _ringHead = static_cast<UInt16>((_ringHead + 1) % MaxBufferRows);
    }

    // clear the new cursor row
    Byte::Fill(
      &_buffer[static_cast<Size>(_cursorRow) * _maxColumns],
      ' ',
      _maxColumns
    );

    _rowLength[_cursorRow] = 0;
    _rowPixelWidth[_cursorRow] = 0;

    if (_doingFullRedraw || _isScrollPending) return;

    if (bufferWrapped) {
      _isScrollPending = true;
    } else if (_getViewportTop() > _previousViewportTop) {
      _isScrollPending = true;
    } else {
      UInt16 newCursorVR = _getCursorVisualRow();
      UInt16 startVR = _getViewportTop();

      if (
        oldCursorVR >= startVR &&
        oldCursorVR < static_cast<UInt16>(startVR + _rows)
      ) {
        _markVisualRowDirty(static_cast<UInt16>(oldCursorVR - startVR));
      }

      if (
        newCursorVR >= startVR &&
        newCursorVR < static_cast<UInt16>(startVR + _rows)
      ) {
        _markVisualRowDirty(static_cast<UInt16>(newCursorVR - startVR));
      }
    }
  }

  void Console::_printPrompt() {
    Write(_prompt);
  }

  void Console::_submitLine() {
    _lineBuffer[_lineIndex] = '\0';

    Execute(_lineBuffer);

    _lineIndex = 0;
    _lineBuffer[0] = '\0';
  }

  void Console::_flush() {
    if (!_lastDirtyRectangle.IsEmpty() && _invalidateCallback) {
      Rectangle dr = _lastDirtyRectangle;
      Int16 shift = _shiftDeltaPixels;

      _lastDirtyRectangle = Rectangle();
      _shiftDeltaPixels = 0;
      _invalidateCallback(_invalidateContext, dr, shift);
    }
  }

  Size Console::_parseLine(
    char* line,
    const char** arguments,
    Size maxArgumentCount
  ) {
    Size argumentCount = 0;
    bool inToken = false;

    for (Size i = 0; line[i] != '\0'; i++) {
      if (line[i] == ' ') {
        line[i] = '\0';
        inToken = false;
      } else if (!inToken) {
        if (argumentCount >= maxArgumentCount) break;

        arguments[argumentCount++] = &line[i];
        inToken = true;
      }
    }

    return argumentCount;
  }
}
