/**
 * @file App/Dialog.cpp
 * @brief Implements @ref @QApp::Dialog.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <AppTypes.hpp>
#include "Dialog.hpp"

namespace Quantum::App {
  DialogButton::DialogButton(
    const char* label,
    DialogButtonCallback onClick
  ) : OnClick(onClick) {
    CString::Copy(label, Label, sizeof(Label));
  }

  Dialog::Dialog(
    Window& parent,
    const char* title,
    const char* header,
    const char* message,
    const DialogButton* buttons,
    Size buttonCount
  ) : _parent(&parent) {
    CString::Copy(title ? title : "", _title, sizeof(_title));
    CString::Copy(header ? header : "", _header, sizeof(_header));
    CString::Copy(message ? message : "", _message, sizeof(_message));

    _buttonCount = buttonCount > MaxButtons ? MaxButtons : buttonCount;

    for (Size buttonIndex = 0; buttonIndex < _buttonCount; ++buttonIndex) {
      _buttons[buttonIndex] = buttons[buttonIndex];
    }
  }

  Dialog::Dialog(
    const char* title,
    const char* header,
    const char* message,
    const DialogButton* buttons,
    Size buttonCount
  ) : _parent(nullptr) {
    CString::Copy(title ? title : "", _title, sizeof(_title));
    CString::Copy(header ? header : "", _header, sizeof(_header));
    CString::Copy(message ? message : "", _message, sizeof(_message));

    _buttonCount = buttonCount > MaxButtons ? MaxButtons : buttonCount;

    for (Size buttonIndex = 0; buttonIndex < _buttonCount; ++buttonIndex) {
      _buttons[buttonIndex] = buttons[buttonIndex];
    }
  }

  Dialog::~Dialog() {
    if (_open) Close();
  }

  void Dialog::Show() {
    // load fonts
    BitmapFont primaryFont = {};
    BitmapFont secondaryFont = {};

    bool hasPrimary = _loadSystemFont(1, primaryFont);
    bool hasSecondary = _loadSystemFont(0, secondaryFont);
    bool hasHeader = _header[0] != '\0';

    // measure header
    UInt16 headerWidth = 0;
    UInt16 headerHeight = 0;

    if (hasHeader && hasPrimary) {
      headerWidth = primaryFont.TextWidth(_header);
      headerHeight = primaryFont.Height;
    } else if (hasHeader) {
      headerWidth = static_cast<UInt16>(CString::Length(_header) * GlyphWidth);
      headerHeight = GlyphHeight;
    }

    // measure message (multi-line)
    UInt16 messageLineCount = 0;
    UInt16 messageMaxWidth = 0;
    UInt8 messageGlyphHeight = hasSecondary ? secondaryFont.Height : GlyphHeight;

    {
      UInt16 lines = 0;
      UInt16 maxWidth = 0;

      if (_message[0] != '\0') {
        lines = 1;

        UInt16 currentWidth = 0;

        for (Size i = 0; _message[i]; ++i) {
          if (_message[i] == '\n') {
            if (currentWidth > maxWidth) maxWidth = currentWidth;

            currentWidth = 0;
            ++lines;
          } else {
            currentWidth = hasSecondary
              ? static_cast<UInt16>(
                  currentWidth
                  + secondaryFont.GetAdvance(static_cast<UInt8>(_message[i]))
                )
              : static_cast<UInt16>(currentWidth + GlyphWidth);
          }
        }

        if (currentWidth > maxWidth) maxWidth = currentWidth;
      }

      messageLineCount = lines;
      messageMaxWidth = maxWidth;
    }

    UInt16 messageHeight = static_cast<UInt16>(
      messageLineCount * messageGlyphHeight
    );
    UInt16 buttonsWidth = static_cast<UInt16>(
      _buttonCount > 0
        ? _buttonCount * ButtonWidth +
          (_buttonCount - 1) * ButtonSpacing
        : 0
    );
    UInt16 contentWidth = messageMaxWidth;

    if (headerWidth > contentWidth) contentWidth = headerWidth;
    if (buttonsWidth > contentWidth) contentWidth = buttonsWidth;

    UInt16 textBlockHeight = 0;

    if (hasHeader) {
      textBlockHeight = static_cast<UInt16>(
        headerHeight + HeaderSpacing + messageHeight
      );
    } else {
      textBlockHeight = messageHeight;
    }

    constexpr UInt16 FrameWidth = 2 * 6;
    constexpr UInt16 FrameHeight = 25 + 6;

    UInt16 neededContentWidth = static_cast<UInt16>(contentWidth + 2 * Padding);
    UInt16 neededContentHeight = static_cast<UInt16>(
      Padding + textBlockHeight + Padding + ButtonHeight + Padding
    );
    UInt16 dialogWidth = static_cast<UInt16>(neededContentWidth + FrameWidth);
    UInt16 dialogHeight = static_cast<UInt16>(
      neededContentHeight + FrameHeight
    );

    if (dialogWidth < 200) dialogWidth = 200;

    Window dialog(
      100,
      100,
      dialogWidth,
      dialogHeight,
      _title,
      BackgroundColor,
      false,
      false,
      false,
      false
    );

    if (!dialog.IsValid()) return;

    _open = true;

    if (_parent) {
      AppServer::SetModal(dialog.GetResourceID(), _parent->GetResourceID());
    }

    Canvas& canvas = dialog.GetCanvas();

    if (!canvas.IsValid()) {
      if (_parent) AppServer::SetModal(dialog.GetResourceID(), 0);

      _open = false;

      return;
    }

    UInt16 surfaceWidth = canvas.GetWidth();
    Int16 cursorY = static_cast<Int16>(Padding);

    // draw header left-aligned in Chicago
    if (hasHeader) {
      if (hasPrimary) canvas.GetPainter().SetFont(primaryFont);

      canvas.GetPainter().DrawText(
        static_cast<Int16>(Padding),
        cursorY,
        _header,
        TextColor,
        BackgroundColor
      );

      cursorY = static_cast<Int16>(cursorY + headerHeight + HeaderSpacing);
    }

    // draw message lines left-aligned in Geneva
    if (hasSecondary) canvas.GetPainter().SetFont(secondaryFont);

    const char* lineStart = _message;

    while (*lineStart) {
      const char* lineEnd = lineStart;

      while (*lineEnd && *lineEnd != '\n') ++lineEnd;

      UInt16 lineLength = static_cast<UInt16>(lineEnd - lineStart);

      if (lineLength > 0) {
        char lineBuffer[256] = {};

        for (UInt16 i = 0; i < lineLength && i < 255; ++i) {
          lineBuffer[i] = lineStart[i];
        }

        lineBuffer[lineLength] = '\0';

        canvas.GetPainter().DrawText(
          static_cast<Int16>(Padding),
          cursorY,
          lineBuffer,
          TextColor,
          BackgroundColor
        );
      }

      cursorY = static_cast<Int16>(cursorY + messageGlyphHeight);

      if (*lineEnd == '\n') {
        lineStart = lineEnd + 1;
      } else {
        break;
      }
    }

    // draw buttons right-aligned with Padding from the right edge
    Int16 buttonY = static_cast<Int16>(cursorY + Padding);
    Int16 buttonsStartX = static_cast<Int16>(
      surfaceWidth - Padding - buttonsWidth
    );

    if (buttonsStartX < 0) buttonsStartX = 0;

    Button* buttons[MaxButtons] = {};

    for (Size i = 0; i < _buttonCount; ++i) {
      Int16 buttonX = static_cast<Int16>(
        buttonsStartX + i * (ButtonWidth + ButtonSpacing)
      );

      buttons[i] = new Button(
        canvas,
        buttonX,
        buttonY,
        ButtonWidth,
        ButtonHeight,
        _buttons[i].Label
      );

      buttons[i]->Draw();
    }

    dialog.Invalidate();

    while (_open) {
      WindowEvent event;

      if (!dialog.GetWindowEvent(&event)) continue;

      switch (event.Type) {
        case WindowEventType::MouseDown: {
          for (Size i = 0; i < _buttonCount; ++i) {
            if (buttons[i]->HandleMouseDown(event.MouseX, event.MouseY)) {
              break;
            }
          }

          dialog.Invalidate();

          break;
        }

        case WindowEventType::MouseUp: {
          for (Size i = 0; i < _buttonCount; ++i) {
            if (buttons[i]->HandleMouseUp(event.MouseX, event.MouseY)) {
              if (_buttons[i].OnClick) _buttons[i].OnClick(*this);

              break;
            }
          }

          dialog.Invalidate();

          break;
        }

        case WindowEventType::MouseMove: {
          for (Size i = 0; i < _buttonCount; ++i) {
            buttons[i]->HandleMouseMove(event.MouseX, event.MouseY);
          }

          dialog.Invalidate();

          break;
        }

        case WindowEventType::Close: {
          Close();

          break;
        }

        default: break;
      }
    }

    if (_parent) AppServer::SetModal(dialog.GetResourceID(), 0);

    for (Size i = 0; i < _buttonCount; ++i) delete buttons[i];
  }

  void Dialog::ShowNonBlocking() {
    BitmapFont primaryFont = {};
    BitmapFont secondaryFont = {};

    bool hasPrimary = _loadSystemFont(1, primaryFont);
    bool hasSecondary = _loadSystemFont(0, secondaryFont);
    bool hasHeader = _header[0] != '\0';

    UInt16 headerWidth = 0;
    UInt16 headerHeight = 0;

    if (hasHeader && hasPrimary) {
      headerWidth = primaryFont.TextWidth(_header);
      headerHeight = primaryFont.Height;
    } else if (hasHeader) {
      headerWidth = static_cast<UInt16>(CString::Length(_header) * GlyphWidth);
      headerHeight = GlyphHeight;
    }

    UInt16 messageLineCount = 0;
    UInt16 messageMaxWidth = 0;
    UInt8 messageGlyphHeight = hasSecondary ? secondaryFont.Height : GlyphHeight;

    {
      UInt16 lines = 0;
      UInt16 maxWidth = 0;

      if (_message[0] != '\0') {
        lines = 1;

        UInt16 currentWidth = 0;

        for (Size i = 0; _message[i]; ++i) {
          if (_message[i] == '\n') {
            if (currentWidth > maxWidth) maxWidth = currentWidth;

            currentWidth = 0;
            ++lines;
          } else {
            currentWidth = hasSecondary
              ? static_cast<UInt16>(
                  currentWidth
                  + secondaryFont.GetAdvance(
                      static_cast<UInt8>(_message[i])
                    )
                )
              : static_cast<UInt16>(currentWidth + GlyphWidth);
          }
        }

        if (currentWidth > maxWidth) maxWidth = currentWidth;
      }

      messageLineCount = lines;
      messageMaxWidth = maxWidth;
    }

    UInt16 messageHeight = static_cast<UInt16>(
      messageLineCount * messageGlyphHeight
    );
    UInt16 buttonsWidth = static_cast<UInt16>(
      _buttonCount > 0
        ? _buttonCount * ButtonWidth +
          (_buttonCount - 1) * ButtonSpacing
        : 0
    );
    UInt16 contentWidth = messageMaxWidth;

    if (headerWidth > contentWidth) contentWidth = headerWidth;
    if (buttonsWidth > contentWidth) contentWidth = buttonsWidth;

    UInt16 textBlockHeight = 0;

    if (hasHeader) {
      textBlockHeight = static_cast<UInt16>(
        headerHeight + HeaderSpacing + messageHeight
      );
    } else {
      textBlockHeight = messageHeight;
    }

    constexpr UInt16 FrameWidth = 2 * 6;
    constexpr UInt16 FrameHeight = 25 + 6;

    UInt16 neededContentWidth = static_cast<UInt16>(contentWidth + 2 * Padding);
    UInt16 neededContentHeight = static_cast<UInt16>(
      Padding + textBlockHeight + Padding + ButtonHeight + Padding
    );
    UInt16 dialogWidth = static_cast<UInt16>(neededContentWidth + FrameWidth);
    UInt16 dialogHeight = static_cast<UInt16>(
      neededContentHeight + FrameHeight
    );

    if (dialogWidth < 200) dialogWidth = 200;

    _dialogWindow = new Window(
      100,
      100,
      dialogWidth,
      dialogHeight,
      _title,
      BackgroundColor,
      false,
      false,
      false,
      false
    );

    if (!_dialogWindow->IsValid()) {
      delete _dialogWindow;
      _dialogWindow = nullptr;

      return;
    }

    _open = true;

    if (_parent) {
      AppServer::SetModal(
        _dialogWindow->GetResourceID(), _parent->GetResourceID()
      );
    }

    Canvas& canvas = _dialogWindow->GetCanvas();

    if (!canvas.IsValid()) {
      Dismiss();

      return;
    }

    // clear with dialog background color before drawing
    canvas.GetPainter().Clear(BackgroundColor);

    UInt16 surfaceWidth = canvas.GetWidth();
    Int16 cursorY = static_cast<Int16>(Padding);

    if (hasHeader) {
      if (hasPrimary) canvas.GetPainter().SetFont(primaryFont);

      canvas.GetPainter().DrawText(
        static_cast<Int16>(Padding),
        cursorY,
        _header,
        TextColor,
        BackgroundColor
      );

      cursorY = static_cast<Int16>(cursorY + headerHeight + HeaderSpacing);
    }

    if (hasSecondary) canvas.GetPainter().SetFont(secondaryFont);

    const char* lineStart = _message;

    while (*lineStart) {
      const char* lineEnd = lineStart;

      while (*lineEnd && *lineEnd != '\n') ++lineEnd;

      UInt16 lineLength = static_cast<UInt16>(lineEnd - lineStart);

      if (lineLength > 0) {
        char lineBuffer[256] = {};

        for (UInt16 i = 0; i < lineLength && i < 255; ++i) {
          lineBuffer[i] = lineStart[i];
        }

        lineBuffer[lineLength] = '\0';

        canvas.GetPainter().DrawText(
          static_cast<Int16>(Padding),
          cursorY,
          lineBuffer,
          TextColor,
          BackgroundColor
        );
      }

      cursorY = static_cast<Int16>(cursorY + messageGlyphHeight);

      if (*lineEnd == '\n') {
        lineStart = lineEnd + 1;
      } else {
        break;
      }
    }

    Int16 buttonY = static_cast<Int16>(cursorY + Padding);
    Int16 buttonsStartX = static_cast<Int16>(
      surfaceWidth - Padding - buttonsWidth
    );

    if (buttonsStartX < 0) buttonsStartX = 0;

    for (Size i = 0; i < _buttonCount; ++i) {
      Int16 buttonX = static_cast<Int16>(
        buttonsStartX + i * (ButtonWidth + ButtonSpacing)
      );

      _buttonElements[i] = new Button(
        canvas,
        buttonX,
        buttonY,
        ButtonWidth,
        ButtonHeight,
        _buttons[i].Label
      );

      _buttonElements[i]->Draw();
    }

    _dialogWindow->Invalidate();
  }

  bool Dialog::TryPumpEvent() {
    if (!_dialogWindow || !_open) return false;

    if (!_dialogWindow->IsValid()) {
      _open = false;

      return false;
    }

    WindowEvent event;

    if (!_dialogWindow->TryGetWindowEvent(&event)) return false;

    switch (event.Type) {
      case WindowEventType::MouseDown: {
        for (Size i = 0; i < _buttonCount; ++i) {
          if (
            _buttonElements[i] &&
            _buttonElements[i]->HandleMouseDown(event.MouseX, event.MouseY)
          ) {
            break;
          }
        }

        _dialogWindow->Invalidate();

        break;
      }

      case WindowEventType::MouseUp: {
        for (Size i = 0; i < _buttonCount; ++i) {
          if (
            _buttonElements[i] &&
            _buttonElements[i]->HandleMouseUp(event.MouseX, event.MouseY)
          ) {
            if (_buttons[i].OnClick) _buttons[i].OnClick(*this);

            break;
          }
        }

        if (_dialogWindow) _dialogWindow->Invalidate();

        break;
      }

      case WindowEventType::MouseMove: {
        for (Size i = 0; i < _buttonCount; ++i) {
          if (_buttonElements[i]) {
            _buttonElements[i]->HandleMouseMove(event.MouseX, event.MouseY);
          }
        }

        _dialogWindow->Invalidate();

        break;
      }

      case WindowEventType::Close: {
        Close();

        break;
      }

      default: break;
    }

    return true;
  }

  void Dialog::Dismiss() {
    if (_parent && _dialogWindow) {
      AppServer::SetModal(_dialogWindow->GetResourceID(), 0);
    }

    for (Size i = 0; i < _buttonCount; ++i) {
      delete _buttonElements[i];
      _buttonElements[i] = nullptr;
    }

    delete _dialogWindow;
    _dialogWindow = nullptr;

    _open = false;
  }

  void Dialog::Close() {
    _open = false;
  }

  bool Dialog::_loadSystemFont(Size index, BitmapFont& font) {
    AppServer::GetSystemFontsResult fonts = {};

    if (
      !AppServer::GetSystemFonts(&fonts) ||
      fonts.FontCount <= index ||
      fonts.Fonts[index].BufferID == 0
    ) {
      return false;
    } else {
      UIntPtr fontAddress = AttachShared(fonts.Fonts[index].BufferID);

      if (fontAddress == 0) {
        return false;
      } else {
        const UInt8* fontData = reinterpret_cast<const UInt8*>(fontAddress);
        Size fontSize = static_cast<Size>(fonts.Fonts[index].DataSize);

        font = QBFParser::Parse(fontData, fontSize);

        if (font.GlyphData) {
          return true;
        } else {
          font = PSFParser::Parse(fontData, fontSize);

          return font.GlyphData != nullptr;
        }
      }
    }
  }
}
