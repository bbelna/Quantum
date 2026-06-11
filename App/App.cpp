/**
 * @file App/App.cpp
 * @brief Implements @ref @QApp::App.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <AppTypes.hpp>
#include <Quantum/Threading/Thread.hpp>

#include "App.hpp"

namespace Quantum::App {
  App::App() {
    _loadSystemFont();
  }

  App::~App() {}

  void App::AddWindow(Window& window) {
    if (_windowCount >= MaxWindows) return;

    for (Size i = 0; i < _windowCount; ++i) {
      if (_windows[i] == &window) return;
    }

    _windows[_windowCount] = &window;
    _windowCount = _windowCount + 1;
  }

  void App::RemoveWindow(Window& window) {
    for (Size i = 0; i < _windowCount; ++i) {
      if (_windows[i] == &window) {
        for (Size j = i + 1; j < _windowCount; ++j) {
          _windows[j - 1] = _windows[j];
        }

        _windowCount = _windowCount - 1;
        _windows[_windowCount] = nullptr;

        return;
      }
    }
  }

  int App::Run() {
    Init();

    if (_windowCount == 0) return -1;

    for (Size i = 0; i < _windowCount; ++i) {
      _windows[i]->DrawAll();
      _windows[i]->Invalidate();
    }

    OnReady();

    while (_running) {
      bool gotEvent = false;

      // snapshot the current window count: hooks may call RemoveWindow
      // (e.g. the default OnClose), but the dispatch loop should still
      // walk to the end of the original list to give every window a
      // chance to process at least one event per tick
      Size windowCount = _windowCount;

      for (Size i = 0; i < windowCount; ++i) {
        Window* window = _windows[i];

        if (!window) continue;

        WindowEvent event;

        if (!window->TryGetWindowEvent(&event)) continue;

        gotEvent = true;

        if (window->DispatchEvent(event)) continue;

        switch (event.Type) {
          case WindowEventType::Close: {
            OnClose(*window);

            break;
          }

          case WindowEventType::Resize: {
            OnResize(*window, event.ContentWidth, event.ContentHeight);

            break;
          }

          case WindowEventType::MouseScroll: {
            OnScroll(*window, event.ScrollDelta);

            break;
          }

          case WindowEventType::Deactivated: {
            OnDeactivated(*window);

            break;
          }

          case WindowEventType::MenuAction: {
            OnMenuAction(*window, event.MenuActionID);

            break;
          }

          default: {
            OnEvent(*window, event);

            break;
          }
        }
      }

      if (!gotEvent) Threading::Thread::Yield();
    }

    return 0;
  }

  void App::OnClose(Window& window) {
    RemoveWindow(window);

    if (_windowCount == 0) RequestExit();
  }

  void App::OnResize(Window& window, UInt16, UInt16) {
    window.GetCanvas().GetPainter().Clear(window.GetContentColor());
    window.DrawAll();
    window.Invalidate();
  }

  void App::OnScroll(Window&, Int8) {}

  void App::OnDeactivated(Window& window) {
    window.ClearFocus();
  }

  void App::OnMenuAction(Window&, UInt32) {}

  void App::OnEvent(Window&, const WindowEvent&) {}

  void App::_loadSystemFont() {
    // only load once per process (static flag)
    if (SystemFontLoaded) return;

    AppServer::GetSystemFontsResult fonts = {};

    if (!AppServer::GetSystemFonts(&fonts)) {
      _kernel.WriteLog(LogLevel::Warning, "GetSystemFonts IPC failed");

      return;
    } else {
      if (
        fonts.FontCount == 0 ||
        fonts.Fonts[0].BufferID == 0 ||
        fonts.Fonts[0].DataSize == 0
      ) {
        _kernel.WriteLog(
          LogLevel::Warning,
          "GetSystemFonts no font (count %u, buffer ID %u, size %u B)",
          static_cast<UInt32>(fonts.FontCount),
          static_cast<UInt32>(fonts.Fonts[0].BufferID),
          fonts.Fonts[0].DataSize
        );

        return;
      } else {
        UIntPtr primaryFontAddress = AttachShared(fonts.Fonts[0].BufferID);
        const UInt8* data = reinterpret_cast<const UInt8*>(
          primaryFontAddress
        );
        Size size = static_cast<Size>(fonts.Fonts[0].DataSize);

        // try QBF first, then PSF
        SystemFont = QBFParser::Parse(data, size);

        if (!SystemFont.GlyphData) {
          SystemFont = PSFParser::Parse(data, size);
        }

        if (!SystemFont.GlyphData) {
          _kernel.WriteLog(LogLevel::Warning, "All font parsers failed");

          DetachShared(primaryFontAddress);

          return;
        } else {
          SharedFontBuffer = primaryFontAddress;
          SystemFontLoaded = true;

          Painter::SetDefaultFont(&SystemFont);

          // load font index 1 for buttons/UI
          if (
            !PrimaryFontLoaded &&
            fonts.FontCount >= 1 &&
            fonts.Fonts[0].BufferID != 0 &&
            fonts.Fonts[0].DataSize != 0
          ) {
            UIntPtr primaryFontAddress = AttachShared(
              fonts.Fonts[0].BufferID
            );

            if (primaryFontAddress != 0) {
              const UInt8* primaryFontData = reinterpret_cast<const UInt8*>(
                primaryFontAddress
              );
              Size primaryFontSize = static_cast<Size>(
                fonts.Fonts[0].DataSize
              );

              PrimaryFont = QBFParser::Parse(
                primaryFontData,
                primaryFontSize
              );

              if (!PrimaryFont.GlyphData) {
                PrimaryFont = PSFParser::Parse(
                  primaryFontData,
                  primaryFontSize
                );
              }

              if (PrimaryFont.GlyphData) {
                PrimaryFontLoaded = true;
              } else {
                DetachShared(primaryFontAddress);
              }

              if (PrimaryFontLoaded) {
                Button::SetFont(&PrimaryFont);
              }
            }
          }
        }
      }
    }
  }
}
