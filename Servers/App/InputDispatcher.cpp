/**
 * @file Servers/App/Core/InputDispatcher.cpp
 * @brief Implements @ref @QAppSrv::InputDispatcher.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <AppServerTypes.hpp>

#include "Compositor.hpp"
#include "Cursor/CursorManager.hpp"
#include "Dock/Dock.hpp"
#include "InputDispatcher.hpp"
#include "Overlays/OverlayManager.hpp"
#include "Windows/WindowManager.hpp"

namespace Quantum::Servers::App {
  namespace {
    // Composites `outer` while leaving the `exclude` sub-rectangle
    // untouched in the composite buffer. Used during shrink drag-resize
    // to repaint the chrome bands (title bar, borders, padding) at the
    // new window edges without overwriting the existing content area
    // pixels — the client's next InvalidateContent lands a clean frame
    // over the preserved region.
    //
    // Splits `outer` by subtracting `exclude`, producing up to four
    // bands: top, bottom, left, right. Each non-empty band is composited
    // via `CompositeStrip`. If the rectangles do not overlap at all, the
    // entire `outer` is composited as a single strip.
    static void CompositeOuterExcluding(
      Compositor& compositor,
      Rectangle outer,
      Rectangle exclude,
      DrawContext& ctx
    ) {
      if (outer.IsEmpty()) return;

      Rectangle clip = exclude.Intersect(outer);

      if (clip.IsEmpty()) {
        compositor.CompositeStrip(outer, ctx);

        return;
      }

      Int16 oL = outer.Origin.X;
      Int16 oT = outer.Origin.Y;
      Int16 oR = outer.GetRight();
      Int16 oB = outer.GetBottom();
      Int16 cL = clip.Origin.X;
      Int16 cT = clip.Origin.Y;
      Int16 cR = clip.GetRight();
      Int16 cB = clip.GetBottom();

      // top band: full width of outer, rows above the excluded region
      if (cT > oT) {
        compositor.CompositeStrip(
          Rectangle(
            oL, oT,
            static_cast<UInt16>(oR - oL),
            static_cast<UInt16>(cT - oT)
          ),
          ctx
        );
      }

      // bottom band: full width of outer, rows below the excluded region
      if (cB < oB) {
        compositor.CompositeStrip(
          Rectangle(
            oL, cB,
            static_cast<UInt16>(oR - oL),
            static_cast<UInt16>(oB - cB)
          ),
          ctx
        );
      }

      // left band: columns to the left of the excluded region, within
      // the excluded region's row range (corners handled by top/bottom)
      if (cL > oL) {
        compositor.CompositeStrip(
          Rectangle(
            oL, cT,
            static_cast<UInt16>(cL - oL),
            static_cast<UInt16>(cB - cT)
          ),
          ctx
        );
      }

      // right band: columns to the right of the excluded region, within
      // the excluded region's row range
      if (cR < oR) {
        compositor.CompositeStrip(
          Rectangle(
            cR, cT,
            static_cast<UInt16>(oR - cR),
            static_cast<UInt16>(cB - cT)
          ),
          ctx
        );
      }
    }
  }

  InputDispatcher::InputDispatcher(
    KernelClient& kernel,
    GraphicsClient& graphics,
    WindowManager& windowManager,
    Compositor& compositor,
    CursorManager& cursorManager,
    UInt16 screenWidth,
    UInt16 screenHeight,
    bool hasHardwareCursor,
    bool hasFastScreenBlit,
    UInt8 bufferBpp
  ) :
    _kernel(kernel),
    _graphics(graphics),
    _windowManager(windowManager),
    _compositor(compositor),
    _cursorManager(cursorManager),
    _screenWidth(screenWidth),
    _screenHeight(screenHeight),
    _hasHardwareCursor(hasHardwareCursor),
    _hasFastScreenBlit(hasFastScreenBlit),
    _bufferBpp(bufferBpp),
    _inputSendHandle(static_cast<IPCPortResourceID>(-1)),
    _inputReplyHandle(static_cast<IPCPortResourceID>(-1)),
    _inputReplyPortID(0)
  {
    _kernel.WaitForIPCPort(Input::ABI::InputPortID);

    _inputSendHandle = _kernel.OpenIPCPort(
      Input::ABI::InputPortID,
      IPCPortRights::Send
    );

    _inputReplyHandle = _kernel.OpenIPCPort(
      static_cast<Kernel::IPC::IPCPortID>(-1),
      IPCPortRights::Manage | IPCPortRights::Receive,
      &_inputReplyPortID
    );
  }

  void InputDispatcher::Dispatch(const InputEvent& event) {
    if (event.Type == InputEventType::MouseMove) {
      _handleMouseMove(event);
    }

    if (event.Type == InputEventType::MouseButtonDown) {
      _handleMouseDown(event);
    }

    if (event.Type == InputEventType::MouseButtonUp) {
      _handleMouseUp(event);
    }

    if (
      event.Type == InputEventType::KeyDown ||
      event.Type == InputEventType::KeyUp ||
      event.Type == InputEventType::MouseScroll
    ) {
      _handleKeyboardScroll(event);
    }
  }

  bool InputDispatcher::GetNextEvent(InputEvent* outEvent) {
    Input::ABI::InputGetNextEventRequest request = {};

    request.ABIVersion = Input::ABI::InputABIVersion;
    request.Operation = Input::ABI::InputOperation::GetNextEvent;
    request.ReplyPortID = _inputReplyPortID;

    _kernel.SendIPCMessage(
      _inputSendHandle,
      static_cast<const void*>(&request),
      sizeof(request)
    );

    IPCMessage* reply = _kernel.ReceiveIPCMessage(_inputReplyHandle);
    bool success = false;

    if (
      reply &&
      reply->PayloadSizeInBytes >= sizeof(Input::ABI::InputGetNextEventResult)
    ) {
      auto* result = static_cast<const Input::ABI::InputGetNextEventResult*>(
        reply->Payload
      );

      if (result->HasEvent && outEvent) {
        *outEvent = result->Event;
        success = true;
      }
    }

    if (reply) {
      free(reply);
    }

    return success;
  }

  bool InputDispatcher::TryGetNextEvent(InputEvent* outEvent) {
    Input::ABI::InputGetNextEventRequest request = {};

    request.ABIVersion = Input::ABI::InputABIVersion;
    request.Operation = Input::ABI::InputOperation::TryGetNextEvent;
    request.ReplyPortID = _inputReplyPortID;

    _kernel.SendIPCMessage(
      _inputSendHandle,
      static_cast<const void*>(&request),
      sizeof(request)
    );

    IPCMessage* reply = _kernel.ReceiveIPCMessage(_inputReplyHandle);
    bool success = false;

    if (
      reply &&
      reply->PayloadSizeInBytes >= sizeof(Input::ABI::InputGetNextEventResult)
    ) {
      auto* result = static_cast<const Input::ABI::InputGetNextEventResult*>(
        reply->Payload
      );

      if (result->HasEvent && outEvent) {
        *outEvent = result->Event;
        success = true;
      }
    }

    if (reply) {
      free(reply);
    }

    return success;
  }

  void InputDispatcher::SendInputRequest() {
    if (_inputRequestPending) return;

    Input::ABI::InputGetNextEventRequest inputReq;

    inputReq.ABIVersion = Input::ABI::InputABIVersion;
    inputReq.Operation = Input::ABI::InputOperation::GetNextEvent;
    inputReq.ReplyPortID = _inputReplyPortID;

    _kernel.SendIPCMessage(
      _inputSendHandle,
      static_cast<const void*>(&inputReq),
      sizeof(inputReq)
    );

    _inputRequestPending = true;
  }

  bool InputDispatcher::IsInputRequestPending() const {
    return _inputRequestPending;
  }

  IPCPortResourceID InputDispatcher::GetInputReplyHandle() const {
    return _inputReplyHandle;
  }

  void InputDispatcher::_handleMouseMove(const InputEvent& event) {
    Point newPos;

    if (_cursorPreUpdated) {
      // cursor was already moved before we acquired the lock
      newPos = _cursorManager.GetPosition();
      _cursorPreUpdated = false;
    } else {
      Point cursor = _cursorManager.GetPosition();

      newPos = Point(
        static_cast<Int16>(cursor.X + event.DeltaX),
        static_cast<Int16>(cursor.Y + event.DeltaY)
      );

      if (newPos.X < 0) newPos.X = 0;
      if (newPos.Y < 0) newPos.Y = 0;

      if (newPos.X >= static_cast<Int16>(_screenWidth)) {
        newPos.X = static_cast<Int16>(_screenWidth - 1);
      }

      if (newPos.Y >= static_cast<Int16>(_screenHeight)) {
        newPos.Y = static_cast<Int16>(_screenHeight - 1);
      }
    }

    // deliver MouseMove to overlays (for hover tracking)
    if (_overlayManager && !_resizing && !_dragging) {
      Overlay* overlay = _overlayManager->HitTest(newPos.X, newPos.Y);

      if (overlay) {
        ABI::WindowEventResult overlayEvent = {};

        overlayEvent.HasEvent = true;
        overlayEvent.Type = ABI::WindowEventType::MouseMove;
        overlayEvent.MouseX = static_cast<Int16>(
          newPos.X - overlay->Frame.Origin.X
        );
        overlayEvent.MouseY = static_cast<Int16>(
          newPos.Y - overlay->Frame.Origin.Y
        );
        overlayEvent.MouseButtons
          = ::Quantum::Core::Enum::ToBase(event.Buttons);

        overlay->DeliverEvent(overlayEvent, _kernel);

        _lastHoveredOverlay = overlay;
      } else if (_lastHoveredOverlay) {
        // mouse left the overlay, send a leave event
        ABI::WindowEventResult leaveEvent = {};

        leaveEvent.HasEvent = true;
        leaveEvent.Type = ABI::WindowEventType::MouseMove;
        leaveEvent.MouseX = -1;
        leaveEvent.MouseY = -1;

        _lastHoveredOverlay->DeliverEvent(leaveEvent, _kernel);
        _lastHoveredOverlay = nullptr;
      }
    }

    if (_resizing && _resizeNode) {
      Window* w = _resizeNode->GetValue();

      // look up per-window minimums. DefaultMinH accounts for the
      // title bar, both horizontal border rows, the interior
      // ContentPadding, and the full outer inset (outer border +
      // Padding band) on top and bottom, plus a 40px content floor
      constexpr UInt16 DefaultMinW = 100;
      const WindowTheme& wTheme = w->GetTheme();
      UInt16 DefaultMinH = static_cast<UInt16>(
        Window::GetTitleBarHeight(wTheme) + wTheme.Border.Thickness
        + 2 * wTheme.ContentPadding
        + 2 * Window::EffectiveOuterInset(false, wTheme)
        + 40
      );

      UInt16 minWLimit = DefaultMinW;
      UInt16 minHLimit = DefaultMinH;
      Size resResIdx = _windowManager.GetResourceCount();

      for (Size ri = 0; ri < _windowManager.GetResourceCount(); ++ri) {
        if (_windowManager.GetResource(ri).Ptr == w) {
          resResIdx = ri;

          if (_windowManager.GetResource(ri).MinWidth > minWLimit) {
            minWLimit = _windowManager.GetResource(ri).MinWidth;
          }

          if (_windowManager.GetResource(ri).MinHeight > minHLimit) {
            minHLimit = _windowManager.GetResource(ri).MinHeight;
          }

          break;
        }
      }

      Int16 dx = static_cast<Int16>(newPos.X - _resizeStart.X);
      Int16 dy = static_cast<Int16>(newPos.Y - _resizeStart.Y);

      Int32 newW32 = static_cast<Int32>(_resizeStartWidth)  + dx;
      Int32 newH32 = static_cast<Int32>(_resizeStartHeight) + dy;

      if (newW32 < minWLimit) newW32 = minWLimit;
      if (newH32 < minHLimit) newH32 = minHLimit;

      UInt16 newW = static_cast<UInt16>(newW32);
      UInt16 newH = static_cast<UInt16>(newH32);

      if (newW != w->GetWidth() || newH != w->GetHeight()) {
        Int16 wx = w->GetX();
        Int16 wy = w->GetY();
        UInt16 oldW = w->GetWidth();
        UInt16 oldH = w->GetHeight();

        // margins account for interior elements that shift when the
        // window dimensions change: title bar buttons (width),
        // resize grip (height), and borders
        UInt16 MarginW = w->GetButtonMarginWidth();
        UInt16 MarginH = static_cast<UInt16>(
          wTheme.Border.Thickness + 8
        );

        // for resize-down vertically, use tight margins, only the
        // border + content padding needs redrawing per mouse move;
        // ContentPadding and the outer Padding band rows sit between
        // the border and content area and must be repainted or stale
        // content pixels bleed through. Horizontal shrink still needs
        // the button margin so the title bar buttons are redrawn at
        // their new positions.
        if (newH < oldH) {
          MarginH
            = static_cast<UInt16>(
                wTheme.Border.Thickness
                + wTheme.ContentPadding
                + Window::EffectiveOuterInset(false, wTheme)
              );
        }

        // capture the content frame before the resize so we can later
        // compute the region that was content before AND is still
        // content after, and skip compositing over those pixels
        Rectangle oldContentFrame = w->GetContentFrame();

        w->SetSize(newW, newH);

        // compute delta strips individually, flushing their
        // bounding-box union would push the entire window through the
        // PCI bus when only the edges actually changed
        Rectangle resizeRightStrip;
        Rectangle resizeBottomStrip;
        Rectangle resizeTitleStrip;
        bool hasRightStrip = false;
        bool hasBottomStrip = false;
        bool hasTitleStrip = false;

        if (newW != oldW) {
          UInt16 mnW = (oldW < newW) ? oldW : newW;
          UInt16 mxW = (oldW > newW) ? oldW : newW;
          UInt16 mxH = (oldH > newH) ? oldH : newH;

          Int16 sx = static_cast<Int16>(wx + mnW - MarginW);
          if (sx < wx) sx = wx;

          resizeRightStrip = Rectangle(
            sx,
            wy,
            static_cast<UInt16>(wx + mxW - sx),
            mxH
          );
          hasRightStrip = true;

          // The title text is centered within the title bar chrome, so
          // it shifts whenever the window width changes. The right strip
          // alone only covers the buttons + right edge and misses the
          // text region; add a title strip covering the title bar area
          // to the LEFT of the right strip so the centered text gets
          // repainted at its new position (and the old text erased)
          // without recompositing the same pixels twice.
          //
          // Strip height covers the full outer inset (outer black
          // border + padding band) and the full title bar height.
          // Without that contribution the bottom rows of the title
          // bar would sit outside the strip and leave stale pixels
          // behind during a width drag.
          if (!w->IsChromeless() && sx > wx) {
            resizeTitleStrip = Rectangle(
              wx,
              wy,
              static_cast<UInt16>(sx - wx),
              static_cast<UInt16>(
                Window::EffectiveOuterInset(false, wTheme)
                + Window::GetTitleBarHeight(wTheme)
              )
            );
            hasTitleStrip = true;
          }
        }

        if (newH != oldH) {
          UInt16 mnH = (oldH < newH) ? oldH : newH;
          UInt16 mxH = (oldH > newH) ? oldH : newH;
          UInt16 mxW = (oldW > newW) ? oldW : newW;
          Int16 sy = static_cast<Int16>(wy + mnH - MarginH);

          if (sy < wy) sy = wy;

          resizeBottomStrip = Rectangle(
            wx,
            sy,
            mxW,
            static_cast<UInt16>(wy + mxH - sy)
          );
          hasBottomStrip = true;
        }

        // notify client of the resize; buffer is only reallocated when
        // the client is provably blocked (immediate path or GetWindowEvent)
        // to avoid freeing a buffer the client is about to AttachShared
        if (resResIdx < _windowManager.GetResourceCount()) {
          // a resize drag can't happen while maximized, so we can
          // pass `false` here unconditionally
          UInt16 cw = Window::ContentWidthFromChromeWidth(
            newW, false, wTheme
          );
          UInt16 ch = Window::ContentHeightFromChromeHeight(
            newH, false, wTheme
          );

          // throttle resize events: only notify the client when the
          // cumulative content size change since the last sent event
          // exceeds a threshold, reducing expensive client redraws
          // during fast resize drags
          constexpr Int32 ResizeThreshold = 1;

          UInt16 lastCW = _windowManager.GetResource(resResIdx).LastSentContentWidth;
          UInt16 lastCH = _windowManager.GetResource(resResIdx).LastSentContentHeight;

          Int32 cwDelta = static_cast<Int32>(cw) - lastCW;
          Int32 chDelta = static_cast<Int32>(ch) - lastCH;

          if (cwDelta < 0) cwDelta = -cwDelta;
          if (chDelta < 0) chDelta = -chDelta;

          bool shouldSendResize
            = (lastCW == 0 && lastCH == 0)
           || (cwDelta + chDelta >= ResizeThreshold);

          if (shouldSendResize) {
            ABI::WindowEventResult result = {};

            result.HasEvent = true;
            result.Type = ABI::WindowEventType::Resize;
            result.ContentWidth = cw;
            result.ContentHeight = ch;
            result.ContentBufferID = 0;
            result.ContentStride = cw;
            result.ContentBytesPerPixel = _bufferBpp;

            if (_windowManager.GetResource(resResIdx).EventReplyPortID != 0) {
              // client is blocking on GetWindowEvent, send dimensions
              // only. The client allocates its own buffer and delivers
              // it back via InvalidateContent after drawing.
              _windowManager.GetResource(resResIdx).DeliverEvent(result, _kernel);
            } else {
              // client not waiting, record dimensions only; the resize
              // event is sent when the client next calls GetWindowEvent
              _windowManager.GetResource(resResIdx).PendingResize        = true;
              _windowManager.GetResource(resResIdx).PendingContentWidth  = cw;
              _windowManager.GetResource(resResIdx).PendingContentHeight = ch;
            }

            _windowManager.GetResource(resResIdx).LastSentContentWidth  = cw;
            _windowManager.GetResource(resResIdx).LastSentContentHeight = ch;
          }
        }

        _cursorManager.MoveTo(newPos);

        if (
          _compositor.GetCompositeBuffer() != nullptr
          && (hasRightStrip || hasBottomStrip || hasTitleStrip)
        ) {
          // wait for the render thread to finish any in-flight composite
          // pass; it drops _stateLock after snapshotting dirty rects and
          // then writes to _compositeBuffer unprotected, so without this
          // wait we race it and the right resize strip can pick up
          // half-written pixels (visible as a glitch on VESA/ViRGE)
          _compositor.WaitForCompositing();

          // ensure any in-flight deferred flush has completed before we
          // write new pixels into the compositor buffer
          _graphics.WaitForPendingFlush();

          Rectangle resizeScreen(0, 0, _screenWidth, _screenHeight);
          DrawContext resizeCtx = _compositor.CreateDrawContext();
          Rectangle wFrame = w->GetFrame();

          // suppress the content-buffer blit for resize strips: the
          // client reuses its shared buffer by reinterpreting it with
          // a new stride on every resize event, so blitting it with the
          // server's stale metadata during a fast drag reads rows at
          // mismatched offsets and produces scrambled pixels. Falling
          // back to the window's content-color fill keeps the strip
          // visually clean until the client's InvalidateContent arrives.
          resizeCtx.SkipContentBlit = true;

          // region that was content before the resize AND is still
          // content after. Preserving these pixels in the composite
          // buffer avoids any visible flash over the client's existing
          // rendered content: on shrink this is the whole new content
          // frame, on grow it is the whole old content frame. The
          // client's subsequent InvalidateContent lands a clean frame
          // over this region once it has finished redrawing. Chrome
          // bands (borders, padding, title bar) sit outside this
          // rectangle and still repaint normally every drag frame.
          Rectangle preservedContent
            = oldContentFrame.Intersect(w->GetContentFrame());

          // composite all strips into the RAM buffer first, then
          // flush the final pixels to VRAM/display in one pass
          // this eliminates flicker from intermediate drawing states
          // (background fill visible before window content) that
          // occurred when compositing directly to VRAM
          Rectangle rcClamped, bcClamped, tcClamped;
          bool hasRC = false, hasBC = false, hasTC = false;

          if (hasTitleStrip) {
            tcClamped = resizeTitleStrip.Intersect(resizeScreen);

            if (!tcClamped.IsEmpty()) {
              hasTC = true;

              // title strip sits entirely above the content frame, so
              // the helper reduces to a plain CompositeStrip here
              CompositeOuterExcluding(
                _compositor, tcClamped, preservedContent, resizeCtx
              );
            }
          }

          if (hasRightStrip) {
            rcClamped = resizeRightStrip.Intersect(resizeScreen);

            Int16 splitX = wFrame.GetRight();

            if (!rcClamped.IsEmpty()) {
              hasRC = true;

              if (
                splitX > rcClamped.Origin.X &&
                splitX < rcClamped.GetRight()
              ) {
                // shrink path: the strip straddles the new right edge.
                // The inner sub-rect is inside the new frame and must
                // skip the content region; the exposed sub-rect is the
                // newly vacated desktop area to the right of the new
                // frame and doesn't overlap any content frame.
                Rectangle inner(
                  rcClamped.Origin.X, rcClamped.Origin.Y,
                  static_cast<UInt16>(splitX - rcClamped.Origin.X),
                  rcClamped.Dimensions.Height
                );
                Rectangle exposed(
                  splitX, rcClamped.Origin.Y,
                  static_cast<UInt16>(
                    rcClamped.GetRight() - splitX
                  ),
                  rcClamped.Dimensions.Height
                );

                CompositeOuterExcluding(
                  _compositor, inner, preservedContent, resizeCtx
                );
                CompositeOuterExcluding(
                  _compositor, exposed, preservedContent, resizeCtx
                );
              } else {
                // grow path: the entire strip is inside the new frame.
                // Exclude the old content region (which is still valid
                // content under the new frame) so existing pixels are
                // preserved; the rest (new chrome, newly-exposed
                // content area inside the grown frame) repaints via
                // the content-color fallback.
                CompositeOuterExcluding(
                  _compositor, rcClamped, preservedContent, resizeCtx
                );
              }
            }
          }

          if (hasBottomStrip) {
            bcClamped = resizeBottomStrip.Intersect(resizeScreen);

            Int16 splitY = wFrame.GetBottom();

            if (!bcClamped.IsEmpty()) {
              hasBC = true;

              if (
                splitY > bcClamped.Origin.Y &&
                splitY < bcClamped.GetBottom()
              ) {
                // shrink path: straddles the new bottom edge
                Rectangle inner(
                  bcClamped.Origin.X, bcClamped.Origin.Y,
                  bcClamped.Dimensions.Width,
                  static_cast<UInt16>(splitY - bcClamped.Origin.Y)
                );
                Rectangle exposed(
                  bcClamped.Origin.X, splitY,
                  bcClamped.Dimensions.Width,
                  static_cast<UInt16>(
                    bcClamped.GetBottom() - splitY
                  )
                );

                CompositeOuterExcluding(
                  _compositor, inner, preservedContent, resizeCtx
                );
                CompositeOuterExcluding(
                  _compositor, exposed, preservedContent, resizeCtx
                );
              } else {
                // grow path: entire strip inside the new frame
                CompositeOuterExcluding(
                  _compositor, bcClamped, preservedContent, resizeCtx
                );
              }
            }
          }

          if (hasTC) {
            _graphics.FlushBackBuffer(
              static_cast<UInt16>(tcClamped.Origin.X),
              static_cast<UInt16>(tcClamped.Origin.Y),
              tcClamped.Dimensions.Width,
              tcClamped.Dimensions.Height,
              !hasRC && !hasBC
            );
          }

          if (hasRC) {
            _graphics.FlushBackBuffer(
              static_cast<UInt16>(rcClamped.Origin.X),
              static_cast<UInt16>(rcClamped.Origin.Y),
              rcClamped.Dimensions.Width,
              rcClamped.Dimensions.Height,
              !hasBC
            );
          }

          if (hasBC) {
            _graphics.FlushBackBuffer(
              static_cast<UInt16>(bcClamped.Origin.X),
              static_cast<UInt16>(bcClamped.Origin.Y),
              bcClamped.Dimensions.Width,
              bcClamped.Dimensions.Height,
              true
            );
          }
        } else {
          if (hasRightStrip) _compositor.Damage(resizeRightStrip);
          if (hasBottomStrip) _compositor.Damage(resizeBottomStrip);
          if (hasTitleStrip) _compositor.Damage(resizeTitleStrip);
        }
      } else {
        _cursorManager.MoveTo(newPos);
      }

      return;
    }

    if (_dragging && _dragNode) {
      Window* w = _dragNode->GetValue();

      // clamp cursor Y so it stays in the draggable area between
      // the menu bar and dock; pure geometry, no window draw dependency
      if (_overlayManager) {
        for (Size index = 0; index < _overlayManager->GetCapacity(); ++index) {
          const Overlay& overlay = _overlayManager->GetOverlay(index);

          if (overlay.ID == 0) continue;

          Int16 minCursorY = static_cast<Int16>(
            overlay.Frame.Origin.Y
            + overlay.Frame.Dimensions.Height
            + _dragOffset.Y
          );

          if (newPos.Y < minCursorY) {
            newPos.Y = minCursorY;
          }

          break;
        }
      }

      if (_dock) {
        Int16 dockTop = static_cast<Int16>(
          _screenHeight - _dock->GetHeight()
        );
        // dock occupies rows [dockTop, screenHeight); the window's
        // bottom-exclusive edge must be <= dockTop, i.e. winY + H <=
        // dockTop, so winY <= dockTop - H. Since winY = cursor -
        // dragOffsetY, the maximum cursor Y is dockTop - H + dragOffsetY.
        Int16 maxCursorY = static_cast<Int16>(
          dockTop - w->GetHeight() + _dragOffset.Y
        );

        if (newPos.Y > maxCursorY) {
          newPos.Y = maxCursorY;
        }
      }

      Int16 newWinX = static_cast<Int16>(newPos.X - _dragOffset.X);
      Int16 newWinY = static_cast<Int16>(newPos.Y - _dragOffset.Y);

      if (newWinX != w->GetX() || newWinY != w->GetY()) {
        Rectangle oldFrame = w->GetFrame();
        Rectangle newFrame(
          newWinX,
          newWinY,
          oldFrame.Dimensions.Width,
          oldFrame.Dimensions.Height
        );

        // Move cursor before any blit/flush so graphics server
        // sees the correct position during overlap checks.
        // Suppress the VRAM flush, the subsequent FlushBackBuffer
        // will flush the cursor region, avoiding a separate VRAM
        // round-trip that causes flicker on software cursor.
        _cursorManager.MoveTo(newPos, true);

        if (_compositor.GetCompositeBuffer() != nullptr && oldFrame.Intersects(newFrame)) {
          // WaitForCompositing BEFORE SetPosition so the render thread
          // finishes compositing the drag-start damage while w is still
          // at oldFrame. Otherwise the render thread (which reads
          // _frame.Origin without holding _stateLock during its compose
          // pass) can see the post-SetPosition new origin while still
          // clipping to the old damage rect. That draws w at its new
          // position clipped to the old bounds, silently dropping the
          // |ddx|/|ddy| leading columns/rows of w and flushing the
          // corrupted pixels to VRAM. The subsequent ScreenBlit then
          // reads those pixels as its source and propagates the
          // missing-edge error across every later drag frame, which
          // shows up as a "cut off" on the leading edge of the motion
          // (right edge when dragging right, top edge when dragging up,
          // etc.). The bug only surfaces when the drag-start bring-to-
          // front path queued pending damage for the render thread; if
          // the window was already frontmost there's nothing for the
          // render thread to race on.
          _compositor.WaitForCompositing();
          _graphics.WaitForPendingFlush();

          w->SetPosition(newWinX, newWinY);

          Int16 ddx = static_cast<Int16>(newWinX - oldFrame.Origin.X);
          Int16 ddy = static_cast<Int16>(newWinY - oldFrame.Origin.Y);
          UInt16 fw = oldFrame.Dimensions.Width;
          UInt16 fh = oldFrame.Dimensions.Height;

          Int16 adx = ddx < 0 ? static_cast<Int16>(-ddx) : ddx;
          Int16 ady = ddy < 0 ? static_cast<Int16>(-ddy) : ddy;

          Rectangle strips[10];
          Size stripCount = 0;

          // phase 1: compute exposed strip geometry

          if (ddy > 0) {
            strips[stripCount++] = Rectangle(
              oldFrame.Origin.X,
              oldFrame.Origin.Y,
              fw,
              ady
            );
          } else if (ddy < 0) {
            strips[stripCount++] = Rectangle(
              oldFrame.Origin.X,
              static_cast<Int16>(oldFrame.GetBottom() - ady),
              fw,
              ady
            );
          }

          Int16 vStripH = static_cast<Int16>(fh - ady);

          if (vStripH > 0 && ddx != 0) {
            Int16 vStripY = ddy > 0
              ? static_cast<Int16>(oldFrame.Origin.Y + ady)
              : oldFrame.Origin.Y;

            if (ddx > 0) {
              strips[stripCount++] = Rectangle(
                oldFrame.Origin.X, vStripY, adx, vStripH
              );
            } else {
              strips[stripCount++] = Rectangle(
                static_cast<Int16>(oldFrame.GetRight() - adx),
                vStripY, adx, vStripH
              );
            }
          }

          Rectangle screen(0, 0, _screenWidth, _screenHeight);
          Rectangle oldOnScreen = oldFrame.Intersect(screen);

          Rectangle shiftedOld(
            static_cast<Int16>(oldOnScreen.Origin.X + ddx),
            static_cast<Int16>(oldOnScreen.Origin.Y + ddy),
            oldOnScreen.Dimensions.Width,
            oldOnScreen.Dimensions.Height
          );
          Rectangle blitDest = shiftedOld.Intersect(screen);
          Rectangle newOnScreen = newFrame.Intersect(screen);

          if (!newOnScreen.IsEmpty()) {
            if (blitDest.IsEmpty()) {
              strips[stripCount++] = newOnScreen;
            } else {
              if (newOnScreen.Origin.X < blitDest.Origin.X) {
                strips[stripCount++] = Rectangle(
                  newOnScreen.Origin.X,
                  newOnScreen.Origin.Y,
                  static_cast<UInt16>(
                    blitDest.Origin.X - newOnScreen.Origin.X
                  ),
                  newOnScreen.Dimensions.Height
                );
              }

              if (newOnScreen.Origin.Y < blitDest.Origin.Y) {
                strips[stripCount++] = Rectangle(
                  newOnScreen.Origin.X,
                  newOnScreen.Origin.Y,
                  newOnScreen.Dimensions.Width,
                  static_cast<UInt16>(
                    blitDest.Origin.Y - newOnScreen.Origin.Y
                  )
                );
              }

              if (newOnScreen.GetRight() > blitDest.GetRight()) {
                strips[stripCount++] = Rectangle(
                  blitDest.GetRight(),
                  newOnScreen.Origin.Y,
                  static_cast<UInt16>(
                    newOnScreen.GetRight() - blitDest.GetRight()
                  ),
                  newOnScreen.Dimensions.Height
                );
              }

              if (newOnScreen.GetBottom() > blitDest.GetBottom()) {
                strips[stripCount++] = Rectangle(
                  newOnScreen.Origin.X,
                  blitDest.GetBottom(),
                  newOnScreen.Dimensions.Width,
                  static_cast<UInt16>(
                    newOnScreen.GetBottom() - blitDest.GetBottom()
                  )
                );
              }
            }
          }

          // phase 2: execute blit + composite + flush

          if (_hasFastScreenBlit) {
            // GPU blit path: the hardware BLT moves the window's
            // pixels on VRAM; skip the CPU-side _bufferBlit, it
            // would copy the full window area in RAM per drag step,
            // which is O(window_area) on the CPU
            // the exposed strip compositing writes directly to the RAM
            // buffer, and a full-window _damage() after drag ends
            // reconciles any remaining staleness

            DrawContext ctx = _compositor.CreateDrawContext();

            for (Size i = 0; i < stripCount; ++i) {
              _compositor.CompositeStrip(strips[i], ctx);
            }

            if (!blitDest.IsEmpty()) {
              if (_compositor.IsDirectFramebuffer()) {
                _graphics.ScreenBlitSync(
                  static_cast<UInt16>(blitDest.Origin.X - ddx),
                  static_cast<UInt16>(blitDest.Origin.Y - ddy),
                  static_cast<UInt16>(blitDest.Origin.X),
                  static_cast<UInt16>(blitDest.Origin.Y),
                  blitDest.Dimensions.Width,
                  blitDest.Dimensions.Height
                );
              } else {
                _graphics.ScreenBlit(
                  static_cast<UInt16>(blitDest.Origin.X - ddx),
                  static_cast<UInt16>(blitDest.Origin.Y - ddy),
                  static_cast<UInt16>(blitDest.Origin.X),
                  static_cast<UInt16>(blitDest.Origin.Y),
                  blitDest.Dimensions.Width,
                  blitDest.Dimensions.Height
                );
              }
            }

            for (Size i = 0; i < stripCount; ++i) {
              Rectangle s = strips[i].Intersect(screen);

              if (!s.IsEmpty()) {
                _graphics.FlushBackBuffer(
                  static_cast<UInt16>(s.Origin.X),
                  static_cast<UInt16>(s.Origin.Y),
                  s.Dimensions.Width,
                  s.Dimensions.Height,
                  i == stripCount - 1
                );
              }
            }
          } else {
            // software path: buffer blit + composite + single flush
            _compositor.BufferBlit(
              oldFrame.Origin.X,
              oldFrame.Origin.Y,
              newFrame.Origin.X,
              newFrame.Origin.Y,
              fw,
              fh
            );

            DrawContext ctx = _compositor.CreateDrawContext();

            for (Size i = 0; i < stripCount; ++i) {
              _compositor.CompositeStrip(strips[i], ctx);
            }

            Rectangle dirty = oldFrame.Union(newFrame).Intersect(screen);

            if (!dirty.IsEmpty()) {
              _graphics.FlushBackBuffer(
                static_cast<UInt16>(dirty.Origin.X),
                static_cast<UInt16>(dirty.Origin.Y),
                dirty.Dimensions.Width,
                dirty.Dimensions.Height,
                true
              );
            }
          }
        } else {
          // fallback: no overlap or no compositor buffer
          // Same ordering as the fast path: wait for the render thread
          // to finish any in-flight compose against oldFrame before we
          // flip w's position, then damage both rects so the next
          // compose sees w at newFrame.
          _compositor.WaitForCompositing();
          w->SetPosition(newWinX, newWinY);
          _compositor.Damage(oldFrame);
          _compositor.Damage(newFrame);
        }

      } else {
        _cursorManager.MoveTo(newPos);
      }

      return;
    }

    if (_pressedButton) {
      bool overButton = _pressedButton->Contains(
        Point(newPos.X, newPos.Y)
      );

      if (overButton != _pressedButton->IsPressed()) {
        _pressedButton->SetPressed(overButton);

        _compositor.Damage(_pressedButton->GetFrame());
      }

      _cursorManager.MoveTo(newPos);

      return;
    }

    if (_mouseCaptureIndex != static_cast<Size>(-1)) {
      Window* w = _windowManager.GetResource(_mouseCaptureIndex).Ptr;

      Rectangle capturedContentFrame = w->GetContentFrame();
      Int16 contentX = static_cast<Int16>(
        newPos.X - capturedContentFrame.Origin.X
      );
      Int16 contentY = static_cast<Int16>(
        newPos.Y - capturedContentFrame.Origin.Y
      );

      ABI::WindowEventResult result = {};

      result.HasEvent = true;
      result.Type = ABI::WindowEventType::MouseMove;
      result.MouseX = contentX;
      result.MouseY = contentY;
      result.MouseButtons = ::Quantum::Core::Enum::ToBase(event.Buttons);

      _windowManager.GetResource(_mouseCaptureIndex).DeliverEvent(result, _kernel);

      _cursorManager.MoveTo(newPos);

      return;
    }

    // hover cursor change: switch to resize cursor over resize handle
    PathNode<Window*>* hoverNode = _windowManager.FindWindowNodeAt(newPos);
    bool overHandle = hoverNode &&
      hoverNode->GetValue()->HitTestResizeHandle(newPos.X, newPos.Y);

    if (overHandle != _hoverOverResize) {
      _hoverOverResize = overHandle;

      _cursorManager.SetResizeCursor(overHandle);
    }

    _cursorManager.MoveTo(newPos);
  }

  void InputDispatcher::_handleMouseDown(const InputEvent& event) {
    if (!::Quantum::Core::Enum::HasAnyFlag(event.Buttons, MouseButton::Left)) {
      return;
    }

    _leftButtonDown = true;

    Point cursor = _cursorManager.GetPosition();

    // test overlays first, they are always above windows
    if (_overlayManager) {
      Overlay* overlay = _overlayManager->HitTest(cursor.X, cursor.Y);

      if (overlay) {
        ABI::WindowEventResult overlayEvent = {};

        overlayEvent.HasEvent = true;
        overlayEvent.Type = ABI::WindowEventType::MouseDown;
        overlayEvent.MouseX = static_cast<Int16>(
          cursor.X - overlay->Frame.Origin.X
        );
        overlayEvent.MouseY = static_cast<Int16>(
          cursor.Y - overlay->Frame.Origin.Y
        );
        overlayEvent.MouseButtons
          = ::Quantum::Core::Enum::ToBase(event.Buttons);

        overlay->DeliverEvent(overlayEvent, _kernel);

        return;
      }

      // click missed all overlays, deliver a MouseDown at (-1,-1)
      // only to overlays that are NOT the first (menu bar) overlay,
      // to close dropdowns without consuming the menu bar's reply port
      bool isFirst = true;

      for (Size index = 0; index < _overlayManager->GetCapacity(); ++index) {
        Overlay& candidate = _overlayManager->GetOverlay(index);

        if (candidate.ID == 0) continue;

        if (isFirst) {
          isFirst = false;

          continue;
        }

        ABI::WindowEventResult outsideClick = {};

        outsideClick.HasEvent = true;
        outsideClick.Type = ABI::WindowEventType::MouseDown;
        outsideClick.MouseX = -1;
        outsideClick.MouseY = -1;

        candidate.DeliverEvent(outsideClick, _kernel);
      }
    }

    // test dock clicks (after overlays, before windows)
    if (_dock && _dock->Contains(cursor.X, cursor.Y)) {
      UInt32 clickedID = _dock->HitTest(cursor.X, cursor.Y);

      if (clickedID != 0) {
        for (Size i = 0; i < _windowManager.GetResourceCount(); ++i) {
          WindowResource& resource = _windowManager.GetResource(i);

          if (resource.ID != clickedID) continue;

          if (resource.Ptr->IsMinimized()) {
            resource.Ptr->SetMinimized(false);

            _compositor.Damage(resource.Ptr->GetVisualBounds());
          }

          // if this window has a modal dialog, bring the dialog instead
          if (resource.ModalDialogID != 0) {
            for (Size di = 0; di < _windowManager.GetResourceCount(); ++di) {
              if (_windowManager.GetResource(di).ID != resource.ModalDialogID) {
                continue;
              }

              PathNode<Window*>* dialogNode
                = _windowManager.GetResource(di).Node;

              _compositor.WaitForCompositing();

              _windowManager.BringToFront(dialogNode);
              _windowManager.EnsureDialogParentBelow(dialogNode);

              PathNode<Window*>* activeNode
                = _windowManager.GetActiveNode();

              if (activeNode && activeNode != dialogNode) {
                activeNode->GetValue()->SetActive(false);

                _windowManager.SendDeactivatedEvent(activeNode);
                _compositor.Damage(activeNode->GetValue()->GetVisualBounds());
              }

              dialogNode->GetValue()->SetActive(true);
              _windowManager.SetActiveNode(dialogNode);
              _compositor.Damage(dialogNode->GetValue()->GetVisualBounds());

              break;
            }
          } else {
            // bring to front and activate
            _compositor.WaitForCompositing();

            _windowManager.BringToFront(resource.Node);

            PathNode<Window*>* activeNode
              = _windowManager.GetActiveNode();

            if (activeNode && activeNode != resource.Node) {
              activeNode->GetValue()->SetActive(false);

              _windowManager.SendDeactivatedEvent(activeNode);
              _compositor.Damage(activeNode->GetValue()->GetVisualBounds());
            }

            resource.Ptr->SetActive(true);
            _windowManager.SetActiveNode(resource.Node);
            _compositor.Damage(resource.Ptr->GetVisualBounds());
          }

          _dock->SetDirty();

          break;
        }
      }

      return;
    }

    PathNode<Window*>* node = _windowManager.FindWindowNodeAt(cursor);

    if (node) {
      Window* w = node->GetValue();

      // modal check: if this window is blocked by a dialog, bring
      // the dialog to front instead of interacting with this window
      bool modalBlocked = false;

      for (Size mi = 0; mi < _windowManager.GetResourceCount(); ++mi) {
        if (_windowManager.GetResource(mi).Ptr != w) continue;

        if (_windowManager.GetResource(mi).ModalDialogID != 0) {
          UInt32 dialogID = _windowManager.GetResource(mi).ModalDialogID;

          for (Size di = 0; di < _windowManager.GetResourceCount(); ++di) {
            if (_windowManager.GetResource(di).ID != dialogID) continue;

            PathNode<Window*>* dialogNode
              = _windowManager.GetResource(di).Node;

            _compositor.WaitForCompositing();

            _windowManager.BringToFront(dialogNode);

            PathNode<Window*>* movedParent
              = _windowManager.EnsureDialogParentBelow(dialogNode);

            if (movedParent) {
              _compositor.Damage(movedParent->GetValue()->GetVisualBounds());
            }

            PathNode<Window*>* activeNode
              = _windowManager.GetActiveNode();

            if (activeNode != dialogNode) {
              if (activeNode) {
                activeNode->GetValue()->SetActive(false);

                _windowManager.SendDeactivatedEvent(activeNode);
                _compositor.Damage(activeNode->GetValue()->GetVisualBounds());
              }

              dialogNode->GetValue()->SetActive(true);

              _windowManager.SetActiveNode(dialogNode);
            }

            _compositor.Damage(dialogNode->GetValue()->GetVisualBounds());

            break;
          }

          modalBlocked = true;
        }

        break;
      }

      if (modalBlocked) {
        // do nothing, interaction blocked
      } else if (w->HitTestCloseButton(cursor.X, cursor.Y)) {
        if (_windowManager.GetActiveNode() != node) {
          if (_windowManager.GetActiveNode()) {
            _windowManager.GetActiveNode()->GetValue()->SetActive(false);

            _windowManager.SendDeactivatedEvent(
              _windowManager.GetActiveNode()
            );
          }

          w->SetActive(true);

          _windowManager.SetActiveNode(node);

          _compositor.WaitForCompositing();

          _windowManager.BringToFront(node);

          PathNode<Window*>* movedParent
            = _windowManager.EnsureDialogParentBelow(node);

          if (movedParent) {
            _compositor.Damage(movedParent->GetValue()->GetVisualBounds());
          }

          _compositor.Damage(w->GetVisualBounds());
        }

        _pressedButton = w->GetCloseButton();
        _pressedButtonNode = node;

        _pressedButton->SetPressed(true);

        _compositor.Damage(_pressedButton->GetFrame());
      } else if (w->HitTestMaximizeButton(cursor.X, cursor.Y)) {
        if (_windowManager.GetActiveNode() != node) {
          if (_windowManager.GetActiveNode()) {
            _windowManager.GetActiveNode()->GetValue()->SetActive(false);

            _windowManager.SendDeactivatedEvent(
              _windowManager.GetActiveNode()
            );
          }

          w->SetActive(true);

          _windowManager.SetActiveNode(node);

          _compositor.WaitForCompositing();

          _windowManager.BringToFront(node);

          PathNode<Window*>* movedParent
            = _windowManager.EnsureDialogParentBelow(node);

          if (movedParent) {
            _compositor.Damage(movedParent->GetValue()->GetVisualBounds());
          }

          _compositor.Damage(w->GetVisualBounds());
        }

        _pressedButton = w->GetMaximizeButton();
        _pressedButtonNode = node;
        _pressedButton->SetPressed(true);

        _compositor.Damage(_pressedButton->GetFrame());
      } else if (w->HitTestMinimizeButton(cursor.X, cursor.Y)) {
        _pressedButton = w->GetMinimizeButton();
        _pressedButtonNode = node;
        _pressedButton->SetPressed(true);

        _compositor.Damage(_pressedButton->GetFrame());
      } else {
        // any other click: bring window to front
        bool wasAlreadyFront
          = (node == _windowManager.GetWindows().GetTail());

        _compositor.WaitForCompositing();

        _windowManager.BringToFront(node);

        PathNode<Window*>* movedParent
          = _windowManager.EnsureDialogParentBelow(node);

        if (movedParent) {
          _compositor.Damage(movedParent->GetValue()->GetVisualBounds());
        }

        if (_windowManager.GetActiveNode() != node) {
          if (_windowManager.GetActiveNode()) {
            _windowManager.GetActiveNode()->GetValue()->SetActive(false);

            _windowManager.SendDeactivatedEvent(
              _windowManager.GetActiveNode()
            );
            _compositor.Damage(
              _windowManager.GetActiveNode()->GetValue()->GetFrame()
            );
          }

          w->SetActive(true);

          _windowManager.SetActiveNode(node);

          _compositor.Damage(w->GetVisualBounds());
        } else if (!wasAlreadyFront) {
          _compositor.Damage(w->GetVisualBounds());
        }

        // check resize handle before title bar
        if (w->HitTestResizeHandle(cursor.X, cursor.Y)) {
          _resizeNode = node;
          _resizing = true;
          // damage the shadow area before suppressing so it gets erased
          _compositor.Damage(w->GetVisualBounds());

          _resizeStart = cursor;
          _resizeStartWidth  = w->GetWidth();
          _resizeStartHeight = w->GetHeight();

          // initialize resize throttle state
          for (Size ri = 0; ri < _windowManager.GetResourceCount(); ++ri) {
            if (_windowManager.GetResource(ri).Ptr != w) continue;

            _windowManager.GetResource(ri).LastSentContentWidth  = 0;
            _windowManager.GetResource(ri).LastSentContentHeight = 0;

            break;
          }
        } else if (w->HitTestTitleBar(cursor.X, cursor.Y)) {
          if (!w->IsMaximized()) {
            // flush pending damage synchronously so the composite
            // buffer reflects the new z-order before the first drag
            // step, without this, BufferBlit copies stale pixels
            // when the render thread hasn't composited yet
            if (!wasAlreadyFront) {
              _compositor.RenderImmediate();
            }

            _dragNode  = node;
            _dragging  = true;
            // damage the shadow area before suppressing so it gets erased
            _compositor.Damage(w->GetVisualBounds());
  
            _dragOffset = Point(
              static_cast<Int16>(cursor.X - w->GetX()),
              static_cast<Int16>(cursor.Y - w->GetY())
            );
          }
        } else {
          // content area click, capture mouse and deliver MouseDown.
          // Convert screen coordinates into content-local coordinates
          // by subtracting the content frame origin, which already
          // accounts for the outer Padding band, the border, the
          // title bar, and the interior ContentPadding.
          Rectangle contentFrame = w->GetContentFrame();
          Int16 contentX = static_cast<Int16>(
            cursor.X - contentFrame.Origin.X
          );
          Int16 contentY = static_cast<Int16>(
            cursor.Y - contentFrame.Origin.Y
          );

          for (Size i = 0; i < _windowManager.GetResourceCount(); ++i) {
            if (_windowManager.GetResource(i).Ptr != w) continue;

            _mouseCaptureIndex = i;

            ABI::WindowEventResult result = {};

            result.HasEvent = true;
            result.Type = ABI::WindowEventType::MouseDown;
            result.MouseX = contentX;
            result.MouseY = contentY;
            result.MouseButtons = ::Quantum::Core::Enum::ToBase(event.Buttons);

            _windowManager.GetResource(i).DeliverEvent(result, _kernel);

            break;
          }
        }

        if (_dock) _dock->SetDirty();
      }
    } else {
      if (_windowManager.GetActiveNode()) {
        _compositor.WaitForCompositing();

        // capture visual bounds before deactivation so the shadow
        // area is included in the damage
        Rectangle oldBounds
          = _windowManager.GetActiveNode()->GetValue()->GetVisualBounds();

        _windowManager.GetActiveNode()->GetValue()->SetActive(false);
        _windowManager.SendDeactivatedEvent(_windowManager.GetActiveNode());
        _compositor.Damage(oldBounds);
        _windowManager.SetActiveNode(nullptr);

        if (_dock) _dock->SetDirty();
      }
    }
  }

  void InputDispatcher::_handleMouseUp(const InputEvent& event) {
    if (::Quantum::Core::Enum::HasAnyFlag(event.Buttons, MouseButton::Left)) {
      return;
    }

    Point cursor = _cursorManager.GetPosition();

    // deliver MouseUp to overlays
    if (_overlayManager) {
      Overlay* overlay = _overlayManager->HitTest(cursor.X, cursor.Y);

      if (overlay) {
        ABI::WindowEventResult overlayEvent = {};

        overlayEvent.HasEvent = true;
        overlayEvent.Type = ABI::WindowEventType::MouseUp;
        overlayEvent.MouseX = static_cast<Int16>(
          cursor.X - overlay->Frame.Origin.X
        );
        overlayEvent.MouseY = static_cast<Int16>(
          cursor.Y - overlay->Frame.Origin.Y
        );
        overlayEvent.MouseButtons
          = ::Quantum::Core::Enum::ToBase(event.Buttons);

        overlay->DeliverEvent(overlayEvent, _kernel);
      }
    }

    if (_pressedButton) {
      bool overButton = _pressedButton->Contains(
        Point(cursor.X, cursor.Y)
      );

      _pressedButton->SetPressed(false);
      _compositor.Damage(_pressedButton->GetFrame());

      if (overButton && _pressedButtonNode) {
        Window* w = _pressedButtonNode->GetValue();

        if (_pressedButton == w->GetCloseButton()) {
          for (Size i = 0; i < _windowManager.GetResourceCount(); ++i) {
            if (_windowManager.GetResource(i).Node != _pressedButtonNode) {
              continue;
            }

            ABI::WindowEventResult closeEvent = {};

            closeEvent.HasEvent = true;
            closeEvent.Type = ABI::WindowEventType::Close;

            _windowManager.GetResource(i).DeliverEvent(closeEvent, _kernel);

            break;
          }

          _pressedButton = nullptr;
          _pressedButtonNode = nullptr;
        } else if (_pressedButton == w->GetMaximizeButton()) {
          _pressedButton = nullptr;
          _pressedButtonNode = nullptr;

          Rectangle oldFrame = w->GetFrame();

          // stretch the maximized window 1px up into the menu bar's
          // bottom chrome border row, and 1px down into the dock's top
          // chrome border row, so the window's own top/bottom border
          // overlaps each chrome line and the visual outline stays
          // unbroken regardless of which one is on top
          UInt16 menuBarBottom = _overlayManager
            ? _overlayManager->GetTopOverlayBottom()
            : 0;
          UInt16 topOffset = menuBarBottom > 0
            ? static_cast<UInt16>(menuBarBottom - 1)
            : 0;

          UInt16 bottomOffset = _dock
            ? static_cast<UInt16>(_dock->GetHeight() - 1)
            : 0;

          w->SetMaximized(
            !w->IsMaximized(), _screenWidth, _screenHeight,
            topOffset, bottomOffset
          );

          _compositor.Damage(oldFrame);
          _compositor.Damage(w->GetVisualBounds());

          for (Size i = 0; i < _windowManager.GetResourceCount(); ++i) {
            if (_windowManager.GetResource(i).Ptr != w) continue;

            UInt16 cw = Window::ContentWidthFromChromeWidth(
              w->GetWidth(), w->IsMaximized(), w->GetTheme()
            );
            UInt16 ch = Window::ContentHeightFromChromeHeight(
              w->GetHeight(), w->IsMaximized(), w->GetTheme()
            );

            ABI::WindowEventResult result = {};

            result.HasEvent = true;
            result.Type = ABI::WindowEventType::Resize;
            result.ContentWidth = cw;
            result.ContentHeight = ch;
            result.ContentBufferID = 0;
            result.ContentStride = cw;
            result.ContentBytesPerPixel = _bufferBpp;

            if (_windowManager.GetResource(i).EventReplyPortID != 0) {
              _windowManager.GetResource(i).DeliverEvent(result, _kernel);
            } else {
              _windowManager.GetResource(i).PendingResize = true;
              _windowManager.GetResource(i).PendingContentWidth = cw;
              _windowManager.GetResource(i).PendingContentHeight = ch;
            }

            break;
          }
        } else if (_pressedButton == w->GetMinimizeButton()) {
          PathNode<Window*>* minimizeNode = _pressedButtonNode;

          _pressedButton = nullptr;
          _pressedButtonNode = nullptr;

          // minimize the window: hide it and activate the next visible one
          // capture visual bounds before deactivation clears the shadow
          Rectangle oldVisualBounds = w->GetVisualBounds();

          w->SetMinimized(true);
          w->SetActive(false);

          _windowManager.SendDeactivatedEvent(minimizeNode);
          _compositor.Damage(oldVisualBounds);

          // find the next visible window to activate
          auto& windows = _windowManager.GetWindows();
          bool foundNext = false;

          for (
            auto* candidate = windows.GetTail();
            candidate;
            candidate = candidate->GetPrevious()
          ) {
            if (candidate->GetValue()->IsMinimized()) continue;
            if (candidate->GetValue()->IsChromeless()) continue;

            candidate->GetValue()->SetActive(true);
            _windowManager.SetActiveNode(candidate);
            _compositor.Damage(candidate->GetValue()->GetVisualBounds());

            foundNext = true;

            break;
          }

          // no visible window left, clear focus entirely
          if (!foundNext) {
            _windowManager.SetActiveNode(nullptr);
          }

          if (_dock) _dock->SetDirty();
        }
      }

      _pressedButton = nullptr;
      _pressedButtonNode = nullptr;
    }

    // send a final resize event if the resize was throttled and the
    // last sent dimensions don't match the current window size
    if (_resizing && _resizeNode) {
      Window* rw = _resizeNode->GetValue();

      for (Size i = 0; i < _windowManager.GetResourceCount(); ++i) {
        if (_windowManager.GetResource(i).Ptr != rw) continue;

        // a resize drag implies the window is not maximized
        UInt16 finalCW = Window::ContentWidthFromChromeWidth(
          rw->GetWidth(), false, rw->GetTheme()
        );
        UInt16 finalCH = Window::ContentHeightFromChromeHeight(
          rw->GetHeight(), false, rw->GetTheme()
        );

        if (
          finalCW != _windowManager.GetResource(i).LastSentContentWidth ||
          finalCH != _windowManager.GetResource(i).LastSentContentHeight
        ) {
          ABI::WindowEventResult result = {};

          result.HasEvent = true;
          result.Type = ABI::WindowEventType::Resize;
          result.ContentWidth = finalCW;
          result.ContentHeight = finalCH;
          result.ContentBufferID = 0;
          result.ContentStride = finalCW;
          result.ContentBytesPerPixel = _bufferBpp;

          if (_windowManager.GetResource(i).EventReplyPortID != 0) {
            _windowManager.GetResource(i).DeliverEvent(result, _kernel);
          } else {
            // client not waiting, mark pending
            _windowManager.GetResource(i).PendingResize = true;
            _windowManager.GetResource(i).PendingContentWidth = finalCW;
            _windowManager.GetResource(i).PendingContentHeight = finalCH;
          }
        }

        _windowManager.GetResource(i).LastSentContentWidth = 0;
        _windowManager.GetResource(i).LastSentContentHeight = 0;

        break;
      }
    }

    // Reconcile the RAM composite buffer after drag, the
    // GPU BLT path skips _bufferBlit during drag for speed,
    // so the RAM buffer is stale at the window's final position
    if (_dragging && _dragNode) {
      _compositor.Damage(_dragNode->GetValue()->GetVisualBounds());
    }

    // Redraw full window decorations to clean up any visual
    // artifacts from tight-margin resize-down strip draws
    if (_resizing && _resizeNode) {
      _compositor.Damage(_resizeNode->GetValue()->GetVisualBounds());
    }

    if (_mouseCaptureIndex != static_cast<Size>(-1)) {
      Size ci = _mouseCaptureIndex;
      Window* w = _windowManager.GetResource(ci).Ptr;

      Rectangle captureContentFrame = w->GetContentFrame();
      Int16 contentX = static_cast<Int16>(
        cursor.X - captureContentFrame.Origin.X
      );
      Int16 contentY = static_cast<Int16>(
        cursor.Y - captureContentFrame.Origin.Y
      );

      ABI::WindowEventResult result = {};

      result.HasEvent = true;
      result.Type = ABI::WindowEventType::MouseUp;
      result.MouseX = contentX;
      result.MouseY = contentY;
      result.MouseButtons = ::Quantum::Core::Enum::ToBase(event.Buttons);

      _windowManager.GetResource(ci).DeliverEvent(result, _kernel);

      _mouseCaptureIndex = static_cast<Size>(-1);
    }

    _leftButtonDown = false;

    if (_dragging && _dragNode) {
      auto* dragWindow = _dragNode->GetValue();

      _compositor.Damage(dragWindow->GetVisualBounds());
    }

    if (_resizing && _resizeNode) {
      auto* resizeWindow = _resizeNode->GetValue();

      _compositor.Damage(resizeWindow->GetVisualBounds());
    }

    _dragging = false;
    _dragNode = nullptr;
    _resizing = false;
    _resizeNode = nullptr;
  }

  void InputDispatcher::_handleKeyboardScroll(const InputEvent& event) {
    // toggle debug flush region outlines with Escape
    if (
      event.Type == InputEventType::KeyDown &&
      event.Scancode == KeyCode::Escape
    ) {
      if (DebugFlushRegion) {
        *DebugFlushRegion = !*DebugFlushRegion;
      }
    }

    // forward keyboard and scroll events to the active (focused) window
    if (
      event.Type == InputEventType::KeyDown ||
      event.Type == InputEventType::KeyUp ||
      event.Type == InputEventType::MouseScroll
    ) {
      PathNode<Window*>* activeNode = _windowManager.GetActiveNode();

      if (activeNode) {
        Window* frontWindow = activeNode->GetValue();

        for (Size i = 0; i < _windowManager.GetResourceCount(); ++i) {
          if (_windowManager.GetResource(i).Ptr != frontWindow) continue;

          ABI::WindowEventResult result = {};

          result.HasEvent = true;

          if (event.Type == InputEventType::MouseScroll) {
            result.Type = ABI::WindowEventType::MouseScroll;
            result.ScrollDelta = event.DeltaZ;
          } else {
            result.Type = ABI::WindowEventType::Keyboard;
            result.KeyboardEvent = event;
          }

          _windowManager.GetResource(i).DeliverEvent(result, _kernel);

          break;
        }
      }
    }
  }
}
