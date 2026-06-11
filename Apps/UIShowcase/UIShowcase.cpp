/**
 * @file Apps/UIShowcase/UIShowcase.cpp
 * @brief Implements @ref @QDemos::UIShowcase::UIShowcase.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "UIShowcase.hpp"

namespace Quantum::Demos::UIShowcase {
  UIShowcase::UIShowcase() :
    _window(
      200,
      160,
      300,
      200,
      "UI Showcase",
      Theme::WindowBackground
    ),
    _heading(
      _window.GetCanvas(),
      16,
      16,
      "UI Showcase Demo"
    ),
    _inputLabel(
      _window.GetCanvas(),
      16,
      76,
      "Text Input"
    ),
    _textInput(
      _window.GetCanvas(),
      16,
      96,
      200
    ),
    _okButton(
      _window.GetCanvas(),
      16,
      40,
      120,
      28,
      "Show Modal!"
    ),
    _cancelButton(
      _window.GetCanvas(),
      112,
      40,
      40,
      28,
      "IDK"
    )
  {
    AddWindow(_window);
  }

  UIShowcase::~UIShowcase() {
    if (_fontBuffer) {
      FreeBlock(_fontBuffer);
    }
  }

  void UIShowcase::Init() {
    _window.SetMinimumSize(120, 80);

    _okButton.SetOnClick([](void* userData) {
      UIShowcase* app = static_cast<UIShowcase*>(userData);

      DialogButton buttons[] = {
        DialogButton(
          "OK",
          [](Dialog& d) {
            d.Close();
          }
        ),
        DialogButton(
          "Cancel",
          [](Dialog& d) {
            d.Close();
          }
        ),
      };

      Dialog dialog(
        app->_window,
        "Message",
        nullptr,
        "Hello from the dialog!",
        buttons,
        2
      );

      dialog.Show();
    }, this);

    _window.Add(_heading);
    _window.Add(_inputLabel);
    _window.Add(_textInput);
    _window.Add(_okButton);
  }
}

QUANTUM_APP(UIShowcase)
