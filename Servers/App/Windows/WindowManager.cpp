/**
 * @file Servers/App/Windows/WindowManager.cpp
 * @brief Implements @ref @QAppSrv::Manager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <AppServerTypes.hpp>

#include "WindowManager.hpp"

namespace Quantum::Servers::App::Windows {
  PathNode<Window*>* WindowManager::FindWindowNodeAt(Point p) const {
    // iterate tail->head (topmost first) to find the frontmost hit
    for (auto* node = _windows.GetTail(); node; node = node->GetPrevious()) {
      if (node->GetValue()->HitTest(p.X, p.Y)) return node;
    }

    return nullptr;
  }

  Pair<Size, WindowResource*> WindowManager::FindResourceByID(UInt32 id) {
    for (Size i = 0; i < _windowResourceCount; ++i) {
      if (_windowResources[i].ID == id) return { i, &_windowResources[i] };
    }

    return { _windowResourceCount, nullptr };
  }

  Pair<Size, WindowResource*> WindowManager::FindResourceByWindow(
    Window* w
  ) {
    for (Size i = 0; i < _windowResourceCount; ++i) {
      if (_windowResources[i].Ptr == w) return { i, &_windowResources[i] };
    }

    return { _windowResourceCount, nullptr };
  }

  void WindowManager::BringToFront(PathNode<Window*>* node) {
    _windows.Remove(node);
    _windows.Append(node);
  }

  void WindowManager::ActivateWindow(PathNode<Window*>* node) {
    _activeNode = node;
    node->GetValue()->SetActive(true);
  }

  void WindowManager::DeactivateCurrentWindow() {
    if (!_activeNode) return;

    _activeNode->GetValue()->SetActive(false);
    SendDeactivatedEvent(_activeNode);
    _activeNode = nullptr;
  }

  void WindowManager::SendDeactivatedEvent(PathNode<Window*>* node) {
    Window* w = node->GetValue();

    for (Size i = 0; i < _windowResourceCount; ++i) {
      if (_windowResources[i].Ptr != w) continue;

      ABI::WindowEventResult result = {};

      result.HasEvent = true;
      result.Type = ABI::WindowEventType::Deactivated;

      _windowResources[i].DeliverEvent(result, _kernel);

      break;
    }
  }

  PathNode<Window*>* WindowManager::EnsureDialogParentBelow(
    PathNode<Window*>* dialogNode
  ) {
    Window* dialogWindow = dialogNode->GetValue();

    for (Size mi = 0; mi < _windowResourceCount; ++mi) {
      if (_windowResources[mi].Ptr != dialogWindow) continue;

      UInt32 dialogID = _windowResources[mi].ID;

      for (Size pi = 0; pi < _windowResourceCount; ++pi) {
        if (_windowResources[pi].ModalDialogID != dialogID) continue;

        PathNode<Window*>* parentNode = _windowResources[pi].Node;

        if (parentNode) {
          _windows.Remove(parentNode);
          _windows.InsertBefore(dialogNode, parentNode);

          return parentNode;
        }

        break;
      }

      break;
    }

    return nullptr;
  }

  void WindowManager::QueueDeleteWindow(Window* w) {
    if (_deleteQueueCount < MaxWindows) _deleteQueue[_deleteQueueCount++] = w;
  }

  void WindowManager::ProcessDeleteQueue() {
    for (Size i = 0; i < _deleteQueueCount; ++i) delete _deleteQueue[i];

    _deleteQueueCount = 0;
  }

  PathNode<Window*>* WindowManager::GetActiveNode() const {
    return _activeNode;
  }

  List<Window*>& WindowManager::GetWindows() {
    return _windows;
  }

  Size WindowManager::GetResourceCount() const {
    return _windowResourceCount;
  }

  WindowResource& WindowManager::GetResource(Size index) {
    return _windowResources[index];
  }

  Size WindowManager::AllocateResource() {
    return _windowResourceCount++;
  }

  void WindowManager::FreeResource(Size index) {
    for (Size j = index; j + 1 < _windowResourceCount; ++j) {
      _windowResources[j] = _windowResources[j + 1];
    }

    _windowResourceCount--;
  }

  UInt32 WindowManager::AllocateResourceID() {
    return _nextWindowResourceID++;
  }

  void WindowManager::SetActiveNode(
    PathNode<Window*>* node,
    bool deferContextUpdate
  ) {
    _activeNode = node;

    if (deferContextUpdate) return;

    // if the newly active window is a modal dialog, don't update the
    // context server's focus, the parent window's menus should remain
    bool isModalDialog = false;

    if (node && node->GetValue()) {
      auto result = FindResourceByWindow(node->GetValue());

      if (result.Second) {
        UInt32 activeID = result.Second->ID;

        for (Size i = 0; i < _windowResourceCount; ++i) {
          if (_windowResources[i].ModalDialogID == activeID) {
            isModalDialog = true;

            break;
          }
        }
      }
    }

    // build the focus context update
    Quantum::Menus::FocusContext focusContext = {};

    if (isModalDialog) {
      // if the newly active window is a modal dialog, clear the focus
      // context so the parent's focus menus don't show
      focusContext.WindowID = 0;
      focusContext.ProcessID = 0;
    } else if (node && node->GetValue()) {
      auto result = FindResourceByWindow(node->GetValue());

      if (result.Second) {
        focusContext.WindowID = result.Second->ID;
        focusContext.ProcessID = result.Second->OwnerProcessID;
      }
    }

    _contextClient.UpdateFocusContext(focusContext);
  }

  void WindowManager::FlushFocusContext() {
    SetActiveNode(_activeNode, false);
  }
}
