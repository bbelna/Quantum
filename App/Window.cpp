/**
 * @file App/Window.cpp
 * @brief Implements @ref @QApp.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <AppTypes.hpp>
#include "Window.hpp"

namespace Quantum::App {
  Window::Window(
    Int16 x,
    Int16 y,
    UInt16 width,
    UInt16 height,
    const char* title,
    UInt32 contentColor,
    bool maximized,
    bool allowClose,
    bool allowMaximize,
    bool allowResize,
    UInt32 innerContentColor
  ) :
    _contentColor(contentColor)
  {
    _valid = _os.Create(
      x,
      y,
      width,
      height,
      title,
      &_canvas,
      contentColor,
      maximized,
      allowClose,
      allowMaximize,
      allowResize,
      innerContentColor
    );
  }

  Window::~Window() {
    if (_valid) {
      _os.Destroy();

      _valid = false;
    }
  }

  UInt32 Window::GetResourceID() const {
    return _os.GetResourceID();
  }

  void Window::SetPosition(Int16 x, Int16 y) {
    if (_valid) _os.SetPosition(x, y);
  }

  void Window::SetSize(UInt16 width, UInt16 height) {
    if (_valid) _os.SetSize(width, height);
  }

  void Window::Invalidate() {
    if (_valid) {
      if (_canvas.IsValid()) {
        _os.Invalidate(
          Rectangle(
            0,
            0,
            _canvas.GetWidth(),
            _canvas.GetHeight()
          )
        );
      } else {
        _os.Invalidate();
      }
    }
  }

  void Window::Invalidate(Rectangle dirtyRectangle) {
    if (_valid) _os.Invalidate(dirtyRectangle);
  }

  void Window::InvalidateWithShift(
    Int16 shiftDeltaY,
    Rectangle dirtyRectangle
  ) {
    if (_valid) _os.InvalidateWithShift(shiftDeltaY, dirtyRectangle);
  }

  bool Window::GetWindowEvent(WindowEvent* outEvent) {
    if (!_valid) {
      return false;
    } else {
      return _os.GetWindowEvent(outEvent, &_canvas);
    }
  }

  bool Window::TryGetWindowEvent(WindowEvent* outEvent) {
    if (!_valid) {
      return false;
    } else {
      return _os.TryGetWindowEvent(outEvent, &_canvas);
    }
  }

  void Window::SetMinimumSize(UInt16 minWidth, UInt16 minHeight) {
    if (_valid) _os.SetMinimumSize(minWidth, minHeight);
  }

  void Window::WaitForClose() {
    if (_valid) {
      _os.WaitForClose();

      _valid = false;
    }
  }

  void Window::Add(Element& element) {
    element._window = this;
    element._next = _firstElement;
    _firstElement = &element;
  }

  void Window::Remove(Element& element) {
    Element** currentElement = &_firstElement;

    while (*currentElement) {
      if (*currentElement == &element) {
        *currentElement = element._next;
        element._next = nullptr;
        element._window = nullptr;

        if (_focused == &element) _focused = nullptr;
        if (_capturedElement == &element) _capturedElement = nullptr;

        return;
      } else {
        currentElement = &(*currentElement)->_next;
      }
    }
  }

  bool Window::DispatchEvent(const WindowEvent& event) {
    switch (event.Type) {
      case WindowEventType::MouseDown: {
        for (
          Element* element = _firstElement;
          element;
          element = element->GetNext()
        ) {
          ClickTarget* clickTarget = element->AsClickTarget();

          if (
            clickTarget &&
            clickTarget->HandleMouseDown(event.MouseX, event.MouseY)
          ) {
            _capturedElement = element;

            if (element->AcceptsKeyboard()) {
              if (_focused != element) {
                if (_focused) _focused->OnFocusChanged(false);

                _focused = element;

                element->OnFocusChanged(true);
              }
            } else {
              ClearFocus();
            }

            return true;
          }
        }

        // clicked empty space; fall back to default focus
        ClearFocus();

        return false;
      }

      case WindowEventType::MouseUp: {
        if (_capturedElement) {
          ClickTarget* clickTarget = _capturedElement->AsClickTarget();

          _capturedElement = nullptr;

          if (clickTarget) {
            clickTarget->HandleMouseUp(event.MouseX, event.MouseY);
          }
          return true;
        } else {
          return false;
        }
      }

      case WindowEventType::MouseMove: {
        if (_capturedElement) {
          ClickTarget* clickTarget = _capturedElement->AsClickTarget();

          if (clickTarget) {
            clickTarget->HandleMouseMove(event.MouseX, event.MouseY);
          }

          return true;
        } else {
          return false;
        }
      }

      case WindowEventType::Keyboard: {
        return _focused
          ? _focused->ProcessKeyEvent(event.KeyboardEvent)
          : false;
      }

      default: return false;
    }
  }

  void Window::DrawAll() {
    for (
      Element* element = _firstElement;
      element;
      element = element->GetNext()
    ) element->Draw();
  }

  void Window::SetFocus(Element& element) {
    if (_focused == &element) {
      return;
    } else {
      if (_focused) {
        _focused->OnFocusChanged(false);
      } else {
        _focused = &element;

        _focused->OnFocusChanged(true);
      }
    }
  }

  void Window::ClearFocus() {
    if (_defaultFocus) {
      SetFocus(*_defaultFocus);
    } else if (_focused) {
      _focused->OnFocusChanged(false);

      _focused = nullptr;
    }
  }

  void Window::SetDefaultFocus(Element& element) {
    _defaultFocus = &element;
  }
}
