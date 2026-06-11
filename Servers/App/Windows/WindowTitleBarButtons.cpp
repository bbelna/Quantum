/**
 * @file Servers/App/Windows/WindowTitleBarButtons.cpp
 * @brief Implements the title bar button container.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "WindowTitleBarButtons.hpp"

namespace Quantum::Servers::App::Windows {
  WindowTitleBarButtons::WindowTitleBarButtons(
    Rectangle* windowFrame
  ) :
    _windowFrame(windowFrame)
  {}

  WindowTitleBarButtons::~WindowTitleBarButtons() {
    PathNode<WindowTitleBarButton*>* node = _buttons.GetHead();

    while (node) {
      PathNode<WindowTitleBarButton*>* next = node->GetNext();

      delete node->GetValue();
      delete node;

      node = next;
    }
  }

  void WindowTitleBarButtons::AddButton(WindowTitleBarButton* button) {
    UInt16 offset = 0;
    PathNode<WindowTitleBarButton*>* node = _buttons.GetHead();

    while (node) {
      offset += node->GetValue()->GetSize() + Gap;
      node = node->GetNext();
    }

    button->SetOffsetFromRight(offset);
    button->SetEdgeInset(_edgeInsetRight, _edgeInsetTop);
    _buttons.Append(new PathNode<WindowTitleBarButton*>(button));
  }

  void WindowTitleBarButtons::SetEdgeInset(UInt16 right, UInt16 top) {
    _edgeInsetRight = right;
    _edgeInsetTop = top;

    PathNode<WindowTitleBarButton*>* node = _buttons.GetHead();

    while (node) {
      node->GetValue()->SetEdgeInset(right, top);
      node = node->GetNext();
    }
  }

  bool WindowTitleBarButtons::Contains(Point p) const {
    return ButtonAt(p) != nullptr;
  }

  WindowTitleBarButton* WindowTitleBarButtons::ButtonAt(Point p) const {
    PathNode<WindowTitleBarButton*>* node = _buttons.GetHead();

    while (node) {
      if (node->GetValue()->Contains(p)) return node->GetValue();

      node = node->GetNext();
    }

    return nullptr;
  }

  Int16 WindowTitleBarButtons::GetLeftEdge() const {
    if (_buttons.IsEmpty()) {
      return static_cast<Int16>(
        _windowFrame->GetRight() - _edgeInsetRight
      );
    }

    return _buttons.GetTail()->GetValue()->GetFrame().Origin.X;
  }
}
