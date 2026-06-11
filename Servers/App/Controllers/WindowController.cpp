/**
 * @file Servers/App/Controllers/WindowController.cpp
 * @brief Implements @ref @QAppSrv::Controllers::WindowController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <AppServerTypes.hpp>

#include "WindowController.hpp"
#include "Compositor.hpp"
#include "Overlays/OverlayManager.hpp"
#include "Windows/WindowManager.hpp"

namespace Quantum::Servers::App::Controllers {
  WindowController::WindowController(
    KernelClient& kernel,
    ServerLog& log,
    WindowManager& windowManager,
    Compositor& compositor,
    GraphicsClient& graphics,
    OverlayManager& overlayManager,
    UInt16 screenWidth,
    UInt16 screenHeight,
    UInt8 bufferBPP,
    bool hasHardwareCursor,
    bool hasFastScreenBlit,
    SharedBufferID primaryFontBufferID,
    UInt32 primaryFontDataSizeInBytes,
    SharedBufferID secondaryFontBufferID,
    UInt32 secondaryFontDataSizeInBytes
  ) :
    RequestController(kernel, log),
    _windows(windowManager),
    _compositor(compositor),
    _graphics(graphics),
    _overlayManager(overlayManager),
    _screenWidth(screenWidth),
    _screenHeight(screenHeight),
    _bufferBPP(bufferBPP),
    _hasHardwareCursor(hasHardwareCursor),
    _hasFastScreenBlit(hasFastScreenBlit),
    _primaryFontBufferID(primaryFontBufferID),
    _primaryFontDataSizeInBytes(primaryFontDataSizeInBytes),
    _secondaryFontBufferID(secondaryFontBufferID),
    _secondaryFontDataSizeInBytes(secondaryFontDataSizeInBytes)
  {
  }

  void WindowController::Handle(
    UInt32 operation,
    const IPCMessage* message
  ) {
    switch (static_cast<ABI::Operation>(operation)) {
      case ABI::Operation::CreateWindow:
        _handleCreateWindow(message);
        _dockDirty = true;
        break;

      case ABI::Operation::CloseWindow:
        _handleCloseWindow(message);
        _dockDirty = true;
        break;

      case ABI::Operation::SetPosition:
        _handleSetPosition(message);
        break;

      case ABI::Operation::SetSize:
        _handleSetSize(message);
        break;

      case ABI::Operation::WaitForClose:
        _handleWaitForClose(message);
        break;

      case ABI::Operation::InvalidateContent:
        _handleInvalidateContent(message);
        break;

      case ABI::Operation::InvalidateContentShift:
        _handleInvalidateContentShift(message);
        break;

      case ABI::Operation::GetWindowEvent:
        _handleGetWindowEvent(message);
        break;

      case ABI::Operation::SetMinimumSize:
        _handleSetMinimumSize(message);
        break;

      case ABI::Operation::SetModal:
        _handleSetModal(message);
        _dockDirty = true;
        break;

      case ABI::Operation::GetSystemFonts:
        _handleGetSystemFonts(message);
        break;

      case ABI::Operation::DeliverMenuAction:
        _handleDeliverMenuAction(message);
        break;

      case ABI::Operation::TryGetWindowEvent:
        _handleTryGetWindowEvent(message);
        break;

      default:
        _log.Warning("Unknown window operation %u", operation);
        break;
    }
  }

  void WindowController::_handleCreateWindow(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes < sizeof(ABI::CreateWindowRequest)
    ) return;

    auto* req = static_cast<const ABI::CreateWindowRequest*>(
      message->Payload
    );

    _log.Trace(
      "Received CreateWindow request (\"%s\", reply port ID %u, %u/%u windows)",
      req->Title,
      static_cast<UInt32>(req->ReplyPortID),
      static_cast<UInt32>(_windows.GetResourceCount()),
      static_cast<UInt32>(WindowManager::MaxWindows)
    );

    if (_windows.GetResourceCount() >= WindowManager::MaxWindows) {
      _log.Error("CreateWindow rejected (max windows reached)");

      ABI::CreateWindowResult failResult = {};

      failResult.Success = false;

      SendReply(req->ReplyPortID, &failResult, sizeof(failResult));

      return;
    }

    Window* window = new Window(
      req->X,
      req->Y,
      req->Width,
      req->Height,
      req->Title,
      _windowTheme,
      req->AllowClose,
      req->AllowMaximize,
      req->AllowResize,
      req->Chromeless
    );

    if (req->ContentColor != 0) {
      window->SetContentColor(req->ContentColor);
    }

    if (req->InnerContentColor != 0) {
      window->SetInnerContentColor(req->InnerContentColor);
    }

    if (req->Maximized) {
      window->SetMaximized(
        true,
        _screenWidth,
        _screenHeight,
        _overlayManager.GetTopOverlayBottom()
      );
    }

    PathNode<Window*>* node = new PathNode<Window*>(window);

    _compositor.WaitForCompositing();

    if (!req->NoFocus) {
      if (_windows.GetActiveNode()) {
        Window* oldWindow = _windows.GetActiveNode()->GetValue();

        oldWindow->SetActive(false);
        _windows.SendDeactivatedEvent(_windows.GetActiveNode());

        // only damage the old window if the new one doesn't fully cover it
        if (!window->GetFrame().Contains(oldWindow->GetFrame())) {
          _compositor.Damage(oldWindow->GetVisualBounds());
        }
      }

      window->SetActive(true);
      _windows.SetActiveNode(node, true);
    }

    _windows.GetWindows().Append(node);

    UInt32 id = _windows.AllocateResourceID();

    // actual content area dimensions for this window (uses the
    // possibly-maximized frame, not the original request size)
    UInt16 activeWidth;
    UInt16 activeHeight;

    if (window->IsChromeless()) {
      activeWidth = static_cast<UInt16>(window->GetWidth());
      activeHeight = static_cast<UInt16>(window->GetHeight());
    } else {
      activeWidth = Window::ContentWidthFromChromeWidth(
        window->GetWidth(), window->IsMaximized(), _windowTheme
      );
      activeHeight = Window::ContentHeightFromChromeHeight(
        window->GetHeight(), window->IsMaximized(), _windowTheme
      );
    }

    // allocate a shared content buffer sized to the content area
    UInt16 bufferStride = activeWidth;
    UInt32 bufferSizeBytes
      = static_cast<UInt32>(bufferStride)
      * activeHeight
      * _bufferBPP;

    SharedBufferID contentBufferID
      = _kernel.CreateSharedBuffer(bufferSizeBytes);
    void* contentBuffer = nullptr;

    if (contentBufferID != 0) {
      UIntPtr bufferAddress = _kernel.AttachSharedBuffer(contentBufferID);

      if (bufferAddress != 0) {
        contentBuffer = reinterpret_cast<void*>(bufferAddress);

        _compositor.InitBuffer(
          contentBuffer,
          static_cast<UInt32>(bufferStride) * activeHeight,
          window->GetContentColor()
        );

        window->SetContentBuffer(
          contentBuffer,
          activeWidth,
          activeHeight,
          bufferStride,
          _bufferBPP
        );
      }
    }

    Size idx = _windows.AllocateResource();

    WindowResource& resource = _windows.GetResource(idx);

    resource = WindowResource{};
    resource.ID = id;
    resource.OwnerProcessID = req->CreatorProcessID;
    resource.Ptr = window;
    resource.Node = node;
    resource.ContentBufferID = contentBufferID;
    resource.ContentBuffer = contentBuffer;

    _compositor.Damage(window->GetVisualBounds());

    ABI::CreateWindowResult result = {};

    result.Success = true;
    result.ID = id;
    result.ContentBufferID = contentBufferID;
    result.ContentWidth = activeWidth;
    result.ContentHeight = activeHeight;
    result.ContentStride = bufferStride;
    result.ContentBytesPerPixel = _bufferBPP;

    _log.Trace(
      "CreateWindow OK: ID %u BufferID %u %ux%u ReplyPort %u",
      id, contentBufferID,
      static_cast<UInt32>(activeWidth),
      static_cast<UInt32>(activeHeight),
      static_cast<UInt32>(req->ReplyPortID)
    );

    SendReply(req->ReplyPortID, &result, sizeof(result));
  }

  void WindowController::_handleCloseWindow(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes < sizeof(ABI::CloseWindowRequest)
    ) return;

    auto* req = static_cast<const ABI::CloseWindowRequest*>(
      message->Payload
    );

    for (Size i = 0; i < _windows.GetResourceCount(); ++i) {
      if (_windows.GetResource(i).ID != req->ID) continue;

      Window* w = _windows.GetResource(i).Ptr;
      PathNode<Window*>* node = _windows.GetResource(i).Node;

      // if this was the active window, activate the next visible one
      if (_windows.GetActiveNode() == node) {
        // defer the context clear, if we find a next window, it will
        // send the real focus update; if not, we clear explicitly
        _windows.SetActiveNode(nullptr, true);

        // walk Z-order backwards to find the next visible window
        auto& windows = _windows.GetWindows();
        bool foundNext = false;

        for (
          auto* candidate = windows.GetTail();
          candidate;
          candidate = candidate->GetPrevious()
        ) {
          if (candidate == node) continue;
          if (candidate->GetValue()->IsMinimized()) continue;
          if (candidate->GetValue()->IsChromeless()) continue;

          candidate->GetValue()->SetActive(true);

          // if the next window is already content-ready, send focus
          // context immediately; otherwise defer until its first
          // invalidate (same as window creation)
          bool defer = !candidate->GetValue()->IsContentReady();

          _windows.SetActiveNode(candidate, defer);
          _compositor.Damage(candidate->GetValue()->GetVisualBounds());

          foundNext = true;

          break;
        }

        // no visible window left, clear focus context now
        if (!foundNext) {
          _windows.FlushFocusContext();
        }
      }

      _compositor.Damage(w->GetVisualBounds());
      _compositor.WaitForCompositing();

      // detach the shared content buffer before deleting the window,
      // otherwise the render thread can access freed memory after the
      // owning process is reaped
      if (_windows.GetResource(i).ContentBuffer) {
        w->SetContentBuffer(nullptr, 0, 0, 0, 4);

        _kernel.DetachSharedBuffer(
          reinterpret_cast<UIntPtr>(
            _windows.GetResource(i).ContentBuffer
          )
        );

        _windows.GetResource(i).ContentBuffer = nullptr;
        _windows.GetResource(i).ContentBufferID = 0;
      }

      _windows.GetWindows().Remove(node);

      delete w;
      delete node;

      // clear any modal relationship referencing this window
      UInt32 closedID = _windows.GetResource(i).ID;

      for (Size mi = 0; mi < _windows.GetResourceCount(); ++mi) {
        if (_windows.GetResource(mi).ModalDialogID == closedID) {
          _windows.GetResource(mi).ModalDialogID = 0;
        }
      }

      _windows.FreeResource(i);

      break;
    }
  }

  void WindowController::_handleSetPosition(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes < sizeof(ABI::SetPositionRequest)
    ) return;

    auto* req = static_cast<const ABI::SetPositionRequest*>(
      message->Payload
    );

    for (Size i = 0; i < _windows.GetResourceCount(); ++i) {
      if (_windows.GetResource(i).ID != req->ID) continue;

      Window* w = _windows.GetResource(i).Ptr;

      _compositor.Damage(w->GetVisualBounds());

      w->SetPosition(req->X, req->Y);

      _compositor.Damage(w->GetVisualBounds());

      break;
    }
  }

  void WindowController::_handleSetSize(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes < sizeof(ABI::SetSizeRequest)
    ) return;

    auto* req = static_cast<const ABI::SetSizeRequest*>(
      message->Payload
    );

    for (Size i = 0; i < _windows.GetResourceCount(); ++i) {
      if (_windows.GetResource(i).ID != req->ID) continue;

      Window* w = _windows.GetResource(i).Ptr;

      _compositor.Damage(w->GetVisualBounds());

      w->SetSize(req->Width, req->Height);

      _compositor.Damage(w->GetVisualBounds());

      break;
    }
  }

  void WindowController::_handleWaitForClose(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes < sizeof(ABI::WaitForCloseRequest)
    ) return;

    auto* req = static_cast<const ABI::WaitForCloseRequest*>(
      message->Payload
    );

    for (Size i = 0; i < _windows.GetResourceCount(); ++i) {
      if (_windows.GetResource(i).ID != req->ID) continue;

      _windows.GetResource(i).CloseReplyPortID = req->ReplyPortID;

      break;
    }
  }

  void WindowController::_handleInvalidateContent(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes < sizeof(ABI::InvalidateContentRequest)
    ) return;

    const ABI::InvalidateContentRequest* req
      = static_cast<const ABI::InvalidateContentRequest*>(
        message->Payload
      );

    for (Size i = 0; i < _windows.GetResourceCount(); ++i) {
      if (_windows.GetResource(i).ID != req->ID) continue;

      // first invalidation marks the window as ready to composite
      if (!_windows.GetResource(i).Ptr->IsContentReady()) {
        _windows.GetResource(i).Ptr->SetContentReady(true);
        _compositor.Damage(
          _windows.GetResource(i).Ptr->GetVisualBounds()
        );

        // flush any deferred focus context update now that the window
        // is visible, this syncs the menu bar with the new window
        if (
          _windows.GetActiveNode()
          && _windows.GetActiveNode()->GetValue()
             == _windows.GetResource(i).Ptr
        ) {
          _windows.FlushFocusContext();
        }
      }

      Window* w = _windows.GetResource(i).Ptr;

      // if the client delivered a new content buffer (post-resize),
      // attach to it and promote it as the active content
      if (req->ContentBufferID != 0) {
        if (_windows.GetResource(i).ContentBuffer) {
          _kernel.DetachSharedBuffer(
            reinterpret_cast<UIntPtr>(
              _windows.GetResource(i).ContentBuffer
            )
          );
        }

        _windows.GetResource(i).ContentBuffer   = nullptr;
        _windows.GetResource(i).ContentBufferID = req->ContentBufferID;

        UIntPtr va = _kernel.AttachSharedBuffer(req->ContentBufferID);

        if (va != 0) {
          _windows.GetResource(i).ContentBuffer
            = reinterpret_cast<void*>(va);

          w->SetContentBuffer(
            _windows.GetResource(i).ContentBuffer,
            req->ContentWidth, req->ContentHeight,
            req->ContentStride, req->ContentBytesPerPixel
          );
        }

        _compositor.Damage(w->GetVisualBounds());
      } else {
        Rectangle contentFrame = w->GetContentFrame();
        Int16 contentX = contentFrame.Origin.X;
        Int16 contentY = contentFrame.Origin.Y;

        // DirtyWidth/Height == 0 means full content area
        UInt16 dirtyWidth = req->DirtyWidth;
        UInt16 dirtyHeight = req->DirtyHeight;

        if (dirtyWidth == 0 || dirtyHeight == 0) {
          _compositor.Damage(w->GetVisualBounds());

          break;
        }

        Rectangle screenDirty(
          static_cast<Int16>(contentX + req->DirtyX),
          static_cast<Int16>(contentY + req->DirtyY),
          dirtyWidth,
          dirtyHeight
        );

        _compositor.Damage(screenDirty.Intersect(w->GetFrame()));
      }

      break;
    }
  }

  void WindowController::_handleInvalidateContentShift(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes
        < sizeof(ABI::InvalidateContentShiftRequest)
    ) return;

    auto* req
      = static_cast<const ABI::InvalidateContentShiftRequest*>(
        message->Payload
      );

    for (Size i = 0; i < _windows.GetResourceCount(); ++i) {
      if (_windows.GetResource(i).ID != req->ID) continue;

      Window* w = _windows.GetResource(i).Ptr;

      Int16 shiftDY = req->ShiftDeltaY;

      Rectangle contentFrame = w->GetContentFrame();
      Int16 contentX = contentFrame.Origin.X;
      Int16 contentY = contentFrame.Origin.Y;
      UInt16 contentW = contentFrame.Dimensions.Width;
      UInt16 contentH = contentFrame.Dimensions.Height;

      Rectangle screenDirty;
      bool hasDirty = false;

      if (req->DirtyWidth > 0 && req->DirtyHeight > 0) {
        screenDirty = Rectangle(
          static_cast<Int16>(contentX + req->DirtyX),
          static_cast<Int16>(contentY + req->DirtyY),
          req->DirtyWidth,
          req->DirtyHeight
        ).Intersect(w->GetFrame());

        hasDirty = !screenDirty.IsEmpty();
      }

      Rectangle contentRect(contentX, contentY, contentW, contentH);
      Rectangle screen(0, 0, _screenWidth, _screenHeight);

      UInt16 absShiftDY = static_cast<UInt16>(
        shiftDY < 0 ? -shiftDY : shiftDY
      );

      if (
        shiftDY == 0 ||
        absShiftDY >= contentH ||
        !screen.Contains(contentRect)
      ) {
        if (hasDirty) _compositor.Damage(screenDirty);

        break;
      }

      PathNode<Window*>* targetNode = _windows.GetResource(i).Node;
      bool occluded = false;

      for (
        auto* above = targetNode->GetNext(); above;
        above = above->GetNext()
      ) {
        if (
          !above->GetValue()->GetFrame().Intersect(contentRect).IsEmpty()
        ) {
          occluded = true;

          break;
        }
      }

      UInt16 overlapH = static_cast<UInt16>(contentH - absShiftDY);

      UInt16 blitSrcY, blitDstY;

      if (shiftDY > 0) {
        blitSrcY = static_cast<UInt16>(contentY + shiftDY);
        blitDstY = static_cast<UInt16>(contentY);
      } else {
        blitSrcY = static_cast<UInt16>(contentY);
        blitDstY = static_cast<UInt16>(contentY + absShiftDY);
      }

      if (_compositor.GetCompositeBuffer() != nullptr && !occluded) {
        _compositor.BufferBlit(
          static_cast<Int16>(contentX),
          static_cast<Int16>(blitSrcY),
          static_cast<Int16>(contentX),
          static_cast<Int16>(blitDstY),
          contentW,
          overlapH
        );

        if (hasDirty) {
          DrawContext ctx = _compositor.CreateDrawContext();

          _compositor.CompositeStrip(screenDirty, ctx);
        }

        if (_hasFastScreenBlit) {
          if (_compositor.IsDirectFramebuffer()) {
            _graphics.ScreenBlitSync(
              static_cast<UInt16>(contentX),
              blitSrcY,
              static_cast<UInt16>(contentX),
              blitDstY,
              contentW,
              overlapH
            );
          } else {
            _graphics.ScreenBlit(
              static_cast<UInt16>(contentX),
              blitSrcY,
              static_cast<UInt16>(contentX),
              blitDstY,
              contentW,
              overlapH
            );
          }

          if (hasDirty) {
            _graphics.FlushBackBuffer(
              static_cast<UInt16>(screenDirty.Origin.X),
              static_cast<UInt16>(screenDirty.Origin.Y),
              screenDirty.Dimensions.Width,
              screenDirty.Dimensions.Height
            );
          }
        } else {
          _graphics.FlushBackBuffer(
            static_cast<UInt16>(contentX),
            static_cast<UInt16>(contentY),
            contentW,
            contentH
          );
        }
      } else {
        if (hasDirty) _compositor.Damage(screenDirty);
      }

      break;
    }
  }

  void WindowController::_handleGetWindowEvent(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes
        < sizeof(ABI::GetWindowEventRequest)
    ) return;

    auto* req = static_cast<const ABI::GetWindowEventRequest*>(
      message->Payload
    );

    bool windowFound = false;

    for (Size i = 0; i < _windows.GetResourceCount(); ++i) {
      if (_windows.GetResource(i).ID != req->ID) continue;

      windowFound = true;
      _windows.GetResource(i).EventReplyPortID = req->ReplyPortID;

      // if a resize happened while the client wasn't waiting,
      // deliver it now
      if (_windows.GetResource(i).PendingResize) {
        UInt16 cw = _windows.GetResource(i).PendingContentWidth;
        UInt16 ch = _windows.GetResource(i).PendingContentHeight;

        // send dimensions only, the client allocates its own buffer
        // and delivers it back via InvalidateContent after drawing
        ABI::WindowEventResult result = {};

        result.HasEvent = true;
        result.Type = ABI::WindowEventType::Resize;
        result.ContentWidth = cw;
        result.ContentHeight = ch;
        result.ContentBufferID = 0;
        result.ContentStride = cw;
        result.ContentBytesPerPixel = _bufferBPP;

        SendReply(req->ReplyPortID, &result, sizeof(result));

        _windows.GetResource(i).PendingResize = false;
        _windows.GetResource(i).EventReplyPortID = 0;
      } else if (_windows.GetResource(i).EventQueueCount > 0) {
        // deliver the oldest queued keyboard/scroll event immediately
        ABI::WindowEventResult queued
          = _windows.GetResource(i).EventQueue[
              _windows.GetResource(i).EventQueueHead
            ];

        _windows.GetResource(i).EventQueueHead = static_cast<Size>(
          (_windows.GetResource(i).EventQueueHead + 1)
          % WindowResource::EventQueueCapacity
        );

        _windows.GetResource(i).EventQueueCount--;

        SendReply(
          _windows.GetResource(i).EventReplyPortID,
          &queued,
          sizeof(queued)
        );

        _windows.GetResource(i).EventReplyPortID = 0;
      }

      break;
    }

    // window was destroyed, tell the client so it doesn't hang
    if (!windowFound) {
      ABI::WindowEventResult closeEvent = {};

      closeEvent.HasEvent = true;
      closeEvent.Type = ABI::WindowEventType::Close;

      SendReply(req->ReplyPortID, &closeEvent, sizeof(closeEvent));
    }
  }

  void WindowController::_handleTryGetWindowEvent(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes
        < sizeof(ABI::GetWindowEventRequest)
    ) return;

    auto* req = static_cast<const ABI::GetWindowEventRequest*>(
      message->Payload
    );

    ABI::WindowEventResult result = {};

    result.HasEvent = false;

    for (Size i = 0; i < _windows.GetResourceCount(); ++i) {
      if (_windows.GetResource(i).ID != req->ID) continue;

      // if a resize happened while the client wasn't waiting,
      // deliver it now
      if (_windows.GetResource(i).PendingResize) {
        result.HasEvent = true;
        result.Type = ABI::WindowEventType::Resize;
        result.ContentWidth = _windows.GetResource(i).PendingContentWidth;
        result.ContentHeight = _windows.GetResource(i).PendingContentHeight;
        result.ContentBufferID = 0;
        result.ContentStride = _windows.GetResource(i).PendingContentWidth;
        result.ContentBytesPerPixel = _bufferBPP;

        _windows.GetResource(i).PendingResize = false;
      } else if (_windows.GetResource(i).EventQueueCount > 0) {
        // deliver the oldest queued event
        result = _windows.GetResource(i).EventQueue[
          _windows.GetResource(i).EventQueueHead
        ];

        _windows.GetResource(i).EventQueueHead = static_cast<Size>(
          (_windows.GetResource(i).EventQueueHead + 1)
          % WindowResource::EventQueueCapacity
        );

        _windows.GetResource(i).EventQueueCount--;
      }

      break;
    }

    // always reply immediately - never store EventReplyPortID
    SendReply(req->ReplyPortID, &result, sizeof(result));
  }

  void WindowController::_handleSetMinimumSize(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes
        < sizeof(ABI::SetMinimumSizeRequest)
    ) return;

    auto* req = static_cast<const ABI::SetMinimumSizeRequest*>(
      message->Payload
    );

    for (Size i = 0; i < _windows.GetResourceCount(); ++i) {
      if (_windows.GetResource(i).ID != req->ID) continue;

      _windows.GetResource(i).MinWidth = req->MinWidth;
      _windows.GetResource(i).MinHeight = req->MinHeight;

      break;
    }
  }

  void WindowController::_handleSetModal(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes < sizeof(ABI::SetModalRequest)
    ) return;

    auto* req = static_cast<const ABI::SetModalRequest*>(
      message->Payload
    );

    if (req->ParentID == 0) {
      for (Size i = 0; i < _windows.GetResourceCount(); ++i) {
        if (_windows.GetResource(i).ModalDialogID != req->DialogID) {
          continue;
        }

        _windows.GetResource(i).ModalDialogID = 0;

        PathNode<Window*>* parentNode
          = _windows.GetResource(i).Node;

        if (parentNode) {
          _compositor.WaitForCompositing();

          if (parentNode != _windows.GetWindows().GetTail()) {
            _windows.BringToFront(parentNode);
            _compositor.Damage(parentNode->GetValue()->GetVisualBounds());
          }

          if (_windows.GetActiveNode() != parentNode) {
            if (_windows.GetActiveNode()) {
              _windows.GetActiveNode()->GetValue()->SetActive(false);
              _windows.SendDeactivatedEvent(
                _windows.GetActiveNode()
              );
              _compositor.Damage(
                _windows.GetActiveNode()->GetValue()->GetFrame()
              );
            }

            parentNode->GetValue()->SetActive(true);
            _windows.SetActiveNode(parentNode);
            _compositor.Damage(parentNode->GetValue()->GetVisualBounds());
          }
        }
      }
    } else {
      for (Size i = 0; i < _windows.GetResourceCount(); ++i) {
        if (_windows.GetResource(i).ID != req->ParentID) continue;

        _windows.GetResource(i).ModalDialogID = req->DialogID;

        break;
      }
    }
  }

  void WindowController::_handleGetSystemFonts(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes < sizeof(ABI::GetSystemFontsRequest)
    ) return;

    const auto* req = reinterpret_cast<const ABI::GetSystemFontsRequest*>(
      message->Payload
    );

    ABI::GetSystemFontsResult result = {};

    result.Success = true;
    result.FontCount = 2;

    // index 0: Default UI font (Helvetica 10)
    result.Fonts[0].BufferID = _secondaryFontBufferID;
    result.Fonts[0].DataSize = _secondaryFontDataSizeInBytes;

    // index 1: Title/system font (Chicago 12)
    result.Fonts[1].BufferID = _primaryFontBufferID;
    result.Fonts[1].DataSize = _primaryFontDataSizeInBytes;

    SendReply(req->ReplyPortID, &result, sizeof(result));
  }

  void WindowController::_handleDeliverMenuAction(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes
        < sizeof(ABI::DeliverMenuActionRequest)
    ) {
      return;
    }

    auto* request = static_cast<const ABI::DeliverMenuActionRequest*>(
      message->Payload
    );

    // ID == 0 means deliver to the currently active window
    UInt32 targetID = request->ID;

    if (targetID == 0 && _windows.GetActiveNode()) {
      auto result = _windows.FindResourceByWindow(
        _windows.GetActiveNode()->GetValue()
      );

      if (result.Second) {
        targetID = result.Second->ID;
      }
    }

    if (targetID == 0) return;

    for (Size index = 0; index < _windows.GetResourceCount(); ++index) {
      if (_windows.GetResource(index).ID != targetID) continue;

      ABI::WindowEventResult event = {};

      event.HasEvent = true;
      event.Type = ABI::WindowEventType::MenuAction;
      event.MenuActionID = request->ActionID;

      _windows.GetResource(index).DeliverEvent(event, _kernel);

      return;
    }
  }
}
