/**
 * @file Apps/About.cpp
 * @brief Implements @ref @QApps::About::About.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "About.hpp"

namespace Quantum::Apps::About {
  About::About() :
    _window(
      150,
      100,
      250,
      113,
      "About",
      Theme::WindowBackground,
      false,
      true,
      false
    ),
    _headerLabel(
      _window.GetCanvas(),
      _padding,
      _padding,
      "Quantum " QUANTUMOS_RELEASE
    ),
    _copyrightLine1(
      _window.GetCanvas(),
      _padding,
      0,
      "Copyright (c) 2025-2026"
    ),
    _copyrightLine2(
      _window.GetCanvas(),
      _padding,
      0,
      "The Quantum Software Project"
    )
  {
    AddWindow(_window);
  }

  void About::Init() {
    UInt16 bodyLineHeight
      = _window
        .GetCanvas()
        .GetPainter()
        .GetFont()
        .Height;
    Int16 cursorY = static_cast<Int16>(
      _padding +
      bodyLineHeight +
      _padding
    );

    _copyrightLine1.SetPosition(
      _padding,
      cursorY
    );

    cursorY = static_cast<Int16>(
      cursorY +
      bodyLineHeight
    );

    _copyrightLine2.SetPosition(
      _padding,
      cursorY
    );

    _window.Add(_headerLabel);
    _window.Add(_copyrightLine1);
    _window.Add(_copyrightLine2);
  }
}

QUANTUM_APP(About)
