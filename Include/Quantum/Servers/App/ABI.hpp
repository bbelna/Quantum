/**
 * @file Include/Quantum/Servers/App/ABI.hpp
 * @brief Declaration of the application server's ABI namespace.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/ABI.hpp>
#include <Quantum/Core/CString.hpp>
#include <Quantum/Input.hpp>
#include <Quantum/Kernel.hpp>
#include <Quantum/Kernel/ABI/Log.hpp>
#include <Quantum/Memory.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::Servers::App::ABI {
  /**
   * @brief ABI version for the application server.
   */
  constexpr UInt32 Version = 1;

  /**
   * @brief IPC port ID for the application server.
   */
  constexpr Kernel::IPC::IPCPortID PortID = 7;

  /**
   * @brief Opaque handle identifying a window managed by the application
   *        server. Assigned by the server on `CreateWindow` and used in
   *        all subsequent operations.
   */
  using WindowResourceID = UInt32;

  /**
   * @brief Operations supported by the application server.
   */
  enum class Operation : UInt32 {
    /**
     * @brief Creates a new window and returns its resource ID.
     */
    CreateWindow = 1,

    /**
     * @brief Closes (destroys) an existing window.
     */
    CloseWindow = 2,

    /**
     * @brief Sets the position of an existing window.
     */
    SetPosition = 3,

    /**
     * @brief Sets the size of an existing window.
     */
    SetSize = 4,

    /**
     * @brief Blocks the caller until the window's close button is clicked.
     *
     * The server stores the reply port and responds when the user clicks
     * the close button, waking the caller so it can clean up and exit.
     */
    WaitForClose = 5,

    /**
     * @brief Marks a window's content area as dirty, triggering a
     *        compositor redraw.
     */
    InvalidateContent = 6,

    /**
     * @brief Blocks the caller until a window event is available for
     *        the specified window. Events include keyboard input
     *        (forwarded only to the focused window) and resize
     *        notifications.
     */
    GetWindowEvent = 7,

    /**
     * @brief Sets the minimum allowed size for a window. The
     *        application server enforces this during user-initiated
     *        resize operations.
     */
    SetMinimumSize = 8,

    /**
     * @brief Invalidates a window's content area with a pixel-shift
     *        hint. The application server can use the shift delta to
     *        move existing pixels via GPU blit instead of re-compositing
     *        the entire area.
     */
    InvalidateContentShift = 9,

    /**
     * @brief Establishes a modal relationship between two windows. While
     *        a modal dialog is active, clicks on the parent window are
     *        blocked and focus is redirected to the dialog. Pass a parent
     *        ID of 0 to clear the modal relationship.
     */
    SetModal = 10,

    /**
     * @brief Retrieves shared buffer IDs for the system fonts. The
     *        caller attaches each buffer and parses it (QBF or PSF) to
     *        obtain a @ref Fonts::BitmapFont.
     */
    GetSystemFonts = 11,

    /**
     * @brief Delivers a menu action event to a specific window. Sent by
     *        the context server when a menu item is invoked by the user.
     */
    DeliverMenuAction = 12,

    /**
     * @brief Creates a system overlay surface rendered above all windows.
     */
    CreateOverlay = 13,

    /**
     * @brief Destroys a system overlay.
     */
    CloseOverlay = 14,

    /**
     * @brief Marks an overlay's content as dirty, triggering a redraw.
     */
    InvalidateOverlay = 15,

    /**
     * @brief Blocks until a mouse event is available for an overlay.
     */
    GetOverlayEvent = 16,

    /**
     * @brief Sets the position of an overlay.
     */
    SetOverlayPosition = 17,

    /**
     * @brief Non-blocking variant of @ref GetWindowEvent. Returns
     *        immediately with `HasEvent = false` when no events are
     *        queued, instead of deferring the reply.
     */
    TryGetWindowEvent = 18
  };

  /**
   * @brief Request to create a new window.
   */
  struct CreateWindowRequest : public ABIRequestWithReplyPort<Operation> {
    /**
     * @brief The x-coordinate of the window's top-left corner.
     */
    Int16 X;

    /**
     * @brief The y-coordinate of the window's top-left corner.
     */
    Int16 Y;

    /**
     * @brief The width of the window in pixels.
     */
    UInt16 Width;

    /**
     * @brief The height of the window in pixels.
     */
    UInt16 Height;

    /**
     * @brief The title text displayed in the title bar.
     */
    char Title[64];

    /**
     * @brief The ARGB32 color used for the content area background and
     *        padding. Pass 0 to use the default (white).
     */
    UInt32 ContentColor;

    /**
     * @brief The ARGB32 color drawn over the client content area before
     *        the app's buffer is blitted. Pass 0 to inherit ContentColor.
     */
    UInt32 InnerContentColor;

    /**
     * @brief If true, the window is created in the maximized state
     *        (filling the entire screen). The `X`, `Y`, `Width`, and
     *        `Height` fields are saved as the restore frame.
     */
    bool Maximized;

    /**
     * @brief If true, the window will have a close button in the title
     *        bar. Defaults to `true`.
     */
    bool AllowClose = true;

    /**
     * @brief If true, the window will have a maximize button in the
     *        title bar. Defaults to `true`.
     */
    bool AllowMaximize = true;

    /**
     * @brief If true, the window can be resized by the user via the
     *        resize grip. Defaults to `true`.
     */
    bool AllowResize = true;

    /**
     * @brief If true, the window is created without any chrome (no title
     *        bar, no border, no resize grip). The entire window frame is
     *        the content area. Used by the context server's menu bar
     *        surface and other system-level overlays.
     */
    bool Chromeless = false;

    /**
     * @brief If true, the window never receives keyboard focus or
     *        activation. It can still receive mouse events. Used for
     *        system overlays like the menu bar that should not steal
     *        focus from application windows.
     */
    bool NoFocus = false;

    /**
     * @brief The process ID of the creator. Set automatically by the
     *        inline @ref CreateWindow function.
     */
    UInt32 CreatorProcessID = 0;
  };

  /**
   * @brief Result of a `CreateWindow` request.
   */
  struct CreateWindowResult {
    /**
     * @brief `true` if the window was created successfully.
     */
    bool Success;

    /**
     * @brief The resource ID of the created window.
     */
    WindowResourceID ID;

    /**
     * @brief Shared buffer ID for the window's content area pixel buffer
     *        (ARGB32). The client attaches via `Memory::AttachShared`.
     */
    SharedBufferID ContentBufferID;

    /**
     * @brief Width of the content area in pixels.
     */
    UInt16 ContentWidth;

    /**
     * @brief Height of the content area in pixels.
     */
    UInt16 ContentHeight;

    /**
     * @brief Row stride of the content buffer in pixels.
     */
    UInt16 ContentStride;

    /**
     * @brief Bytes per pixel of the content buffer (2 for RGB565,
     *        4 for ARGB32).
     */
    UInt8 ContentBytesPerPixel;
  };

  /**
   * @brief Request to close an existing window.
   */
  struct CloseWindowRequest : public ABIRequest<Operation> {
    /**
     * @brief The resource ID of the window to close.
     */
    WindowResourceID ID;
  };

  /**
   * @brief Request to set the position of an existing window.
   */
  struct SetPositionRequest : public ABIRequest<Operation> {
    /**
     * @brief The resource ID of the window to reposition.
     */
    WindowResourceID ID;

    /**
     * @brief The new x-coordinate of the window's top-left corner.
     */
    Int16 X;

    /**
     * @brief The new y-coordinate of the window's top-left corner.
     */
    Int16 Y;
  };

  /**
   * @brief Request to set the size of an existing window.
   */
  struct SetSizeRequest : public ABIRequest<Operation> {
    /**
     * @brief The resource ID of the window to resize.
     */
    WindowResourceID ID;

    /**
     * @brief The new width of the window in pixels.
     */
    UInt16 Width;

    /**
     * @brief The new height of the window in pixels.
     */
    UInt16 Height;
  };

  /**
   * @brief Request to block until a window's close button is clicked.
   */
  struct WaitForCloseRequest
    : public ABIRequestWithReplyPort<Operation> {
    /**
     * @brief The resource ID of the window to wait on.
     */
    WindowResourceID ID;
  };

  /**
   * @brief Request to mark a window's content as dirty.
   */
  struct InvalidateContentRequest : public ABIRequest<Operation> {
    /**
     * @brief The resource ID of the window to invalidate.
     */
    WindowResourceID ID;

    /**
     * @brief Content-relative dirty rectangle. If Width and Height are
     *        both 0, the entire content area is invalidated.
     */
    UInt16 DirtyX;
    UInt16 DirtyY;
    UInt16 DirtyWidth;
    UInt16 DirtyHeight;

    /**
     * @brief Optional new content buffer allocated by the client after a
     *        resize. When non-zero the server attaches to this buffer and
     *        promotes it as the window's active content.
     */
    SharedBufferID ContentBufferID = 0;
    UInt16 ContentWidth = 0;
    UInt16 ContentHeight = 0;
    UInt16 ContentStride = 0;
    UInt8 ContentBytesPerPixel = 0;
  };

  /**
   * @brief The type of event delivered by `GetWindowEvent`.
   */
  enum class WindowEventType : UInt8 {
    /**
     * @brief A keyboard input event.
     */
    Keyboard = 1,

    /**
     * @brief The window was resized by the user. The new content
     *        dimensions are provided so the client can re-layout.
     */
    Resize = 2,

    /**
     * @brief A mouse scroll wheel event directed at this window.
     */
    MouseScroll = 3,

    /**
     * @brief The window has been closed by the user. The client should
     *        clean up and exit.
     */
    Close = 4,

    /**
     * @brief A mouse button was pressed inside the window's content area.
     */
    MouseDown = 5,

    /**
     * @brief A mouse button was released. Delivered to the window that
     *        received the original MouseDown, regardless of cursor position.
     */
    MouseUp = 6,

    /**
     * @brief The mouse cursor moved while a button is held. Delivered to
     *        the window that received the original MouseDown.
     */
    MouseMove = 7,

    /**
     * @brief The window lost focus (another window became active).
     */
    Deactivated = 8,

    /**
     * @brief A menu action was invoked by the user via the system menu
     *        bar. The action ID is in @ref WindowEventResult::MenuActionID.
     */
    MenuAction = 9
  };

  /**
   * @brief Request to block until a window event is available.
   */
  struct GetWindowEventRequest
    : public ABIRequestWithReplyPort<Operation> {
    /**
     * @brief The resource ID of the window to receive events for.
     */
    WindowResourceID ID;
  };

  /**
   * @brief Result of a `GetWindowEvent` request.
   */
  struct WindowEventResult {
    /**
     * @brief `true` if an event is present; `false` on error.
     */
    bool HasEvent;

    /**
     * @brief The type of window event.
     */
    WindowEventType Type;

    /**
     * @brief The keyboard input event. Valid only when
     *        `Type == WindowEventType::Keyboard`.
     */
    Quantum::Input::InputEvent KeyboardEvent;

    /**
     * @brief New content width in pixels. Valid only when
     *        `Type == WindowEventType::Resize`.
     */
    UInt16 ContentWidth;

    /**
     * @brief New content height in pixels. Valid only when
     *        `Type == WindowEventType::Resize`.
     */
    UInt16 ContentHeight;

    /**
     * @brief Scroll wheel delta. Positive = scroll up, negative =
     *        scroll down. Valid only when
     *        `Type == WindowEventType::MouseScroll`.
     */
    Int8 ScrollDelta;

    /**
     * @brief New content buffer ID. Valid only when
     *        `Type == WindowEventType::Resize`. The client must detach the
     *        old buffer and attach this one before drawing.
     */
    SharedBufferID ContentBufferID;

    /**
     * @brief Row stride of the new content buffer in pixels. Valid only
     *        when `Type == WindowEventType::Resize`.
     */
    UInt16 ContentStride;

    /**
     * @brief Bytes per pixel of the new content buffer (2 for RGB565,
     *        4 for ARGB32). Valid only when
     *        `Type == WindowEventType::Resize`.
     */
    UInt8 ContentBytesPerPixel;

    /**
     * @brief Content-relative X coordinate of the mouse cursor. Valid for
     *        MouseDown, MouseUp, and MouseMove events.
     */
    Int16 MouseX;

    /**
     * @brief Content-relative Y coordinate of the mouse cursor. Valid for
     *        MouseDown, MouseUp, and MouseMove events.
     */
    Int16 MouseY;

    /**
     * @brief Mouse button flags (see `Input::MouseButton`). Valid for
     *        MouseDown, MouseUp, and MouseMove events.
     */
    UInt8 MouseButtons;

    /**
     * @brief The action ID invoked from the menu bar. Valid only when
     *        `Type == WindowEventType::MenuAction`.
     */
    UInt32 MenuActionID;
  };

  /**
   * @brief Request to set the minimum allowed size for a window.
   */
  struct SetMinimumSizeRequest : public ABIRequest<Operation> {
    /**
     * @brief The resource ID of the window.
     */
    WindowResourceID ID;

    /**
     * @brief The minimum width in pixels.
     */
    UInt16 MinWidth;

    /**
     * @brief The minimum height in pixels.
     */
    UInt16 MinHeight;
  };

  /**
   * @brief Request to invalidate a window's content area with a
   *        pixel-shift hint for compositor optimization.
   */
  struct InvalidateContentShiftRequest
    : public ABIRequest<Operation> {
    /**
     * @brief The resource ID of the window.
     */
    WindowResourceID ID;

    /**
     * @brief Signed pixel shift delta. Positive = content shifted up,
     *        negative = content shifted down.
     */
    Int16 ShiftDeltaY;

    /**
     * @brief Content-relative dirty rectangle (the newly exposed strip).
     */
    UInt16 DirtyX;
    UInt16 DirtyY;
    UInt16 DirtyWidth;
    UInt16 DirtyHeight;
  };

  /**
   * @brief Request to set a modal relationship between two windows.
   */
  struct SetModalRequest : public ABIRequest<Operation> {
    /**
     * @brief The resource ID of the dialog (modal) window.
     */
    WindowResourceID DialogID;

    /**
     * @brief The resource ID of the parent window to block, or 0 to
     *        clear the modal relationship for the dialog.
     */
    WindowResourceID ParentID;
  };

  /**
   * @brief Request to deliver a menu action event to a window. Sent by
   *        the context server when a user invokes a menu item.
   */
  struct DeliverMenuActionRequest : public ABIRequest<Operation> {
    /**
     * @brief The resource ID of the window to deliver the event to.
     */
    WindowResourceID ID;

    /**
     * @brief The action ID to deliver.
     */
    UInt32 ActionID;
  };

  // -----------------------------------------------------------------------
  // Overlay types
  // -----------------------------------------------------------------------

  /**
   * @brief Unique identifier for an overlay resource.
   */
  using OverlayResourceID = UInt32;

  /**
   * @brief Request to create a system overlay.
   */
  struct CreateOverlayRequest
    : public ABIRequestWithReplyPort<Operation> {
    /**
     * @brief The x-coordinate of the overlay's top-left corner.
     */
    Int16 X;

    /**
     * @brief The y-coordinate of the overlay's top-left corner.
     */
    Int16 Y;

    /**
     * @brief The width of the overlay in pixels.
     */
    UInt16 Width;

    /**
     * @brief The height of the overlay in pixels.
     */
    UInt16 Height;
  };

  /**
   * @brief Result of a @ref Operation::CreateOverlay request.
   */
  struct CreateOverlayResult {
    /**
     * @brief `true` if the overlay was created successfully.
     */
    bool Success;

    /**
     * @brief The assigned overlay resource ID.
     */
    OverlayResourceID ID;

    /**
     * @brief Shared buffer ID for the overlay's content buffer.
     */
    SharedBufferID ContentBufferID;

    /**
     * @brief Content width in pixels.
     */
    UInt16 ContentWidth;

    /**
     * @brief Content height in pixels.
     */
    UInt16 ContentHeight;

    /**
     * @brief Content stride in pixels.
     */
    UInt16 ContentStride;

    /**
     * @brief Bytes per pixel of the content buffer.
     */
    UInt8 ContentBytesPerPixel;

  };

  /**
   * @brief Request to close an overlay.
   */
  struct CloseOverlayRequest : public ABIRequest<Operation> {
    /**
     * @brief The overlay resource ID to close.
     */
    OverlayResourceID ID;
  };

  /**
   * @brief Request to invalidate an overlay's content.
   */
  struct InvalidateOverlayRequest : public ABIRequest<Operation> {
    /**
     * @brief The overlay resource ID to invalidate.
     */
    OverlayResourceID ID;
  };

  /**
   * @brief Request to block until a mouse event is available for an overlay.
   */
  struct GetOverlayEventRequest
    : public ABIRequestWithReplyPort<Operation> {
    /**
     * @brief The overlay resource ID to receive events for.
     */
    OverlayResourceID ID;
  };

  /**
   * @brief Request to set the position of an overlay.
   */
  struct SetOverlayPositionRequest : public ABIRequest<Operation> {
    /**
     * @brief The overlay resource ID.
     */
    OverlayResourceID ID;

    /**
     * @brief The new x-coordinate.
     */
    Int16 X;

    /**
     * @brief The new y-coordinate.
     */
    Int16 Y;
  };

  // -----------------------------------------------------------------------
  // Overlay inline client functions
  // -----------------------------------------------------------------------

  /**
   * @brief Creates a system overlay.
   * @param x The x-coordinate of the overlay.
   * @param y The y-coordinate of the overlay.
   * @param width The width in pixels.
   * @param height The height in pixels.
   * @param outID Pointer to receive the overlay ID.
   * @param outResult Optional pointer to receive the full result.
   * @return `true` if the overlay was created.
   */
  inline bool CreateOverlay(
    Int16 x,
    Int16 y,
    UInt16 width,
    UInt16 height,
    OverlayResourceID* outID,
    CreateOverlayResult* outResult = nullptr
  ) {
    using namespace Kernel;

    IPCPortID replyPortID = static_cast<IPCPortID>(
      450 + Kernel::ABI::Process::GetID() * 3
    );

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return false;

    IPCPortResourceID replyHandle = Kernel::ABI::IPC::Open(
      replyPortID,
      IPCPortRights::Manage | IPCPortRights::Receive
    );

    if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::IPC::Close(sendHandle);

      return false;
    }

    CreateOverlayRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::CreateOverlay;
    request.ReplyPortID = replyPortID;
    request.X = x;
    request.Y = y;
    request.Width = width;
    request.Height = height;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);

    IPCMessage* reply = Kernel::ABI::IPC::Receive(replyHandle);

    bool success = false;

    if (reply && reply->PayloadSizeInBytes >= sizeof(CreateOverlayResult)) {
      auto* result = static_cast<const CreateOverlayResult*>(reply->Payload);

      if (result->Success) {
        if (outID) {
          *outID = result->ID;
        }

        if (outResult) {
          *outResult = *result;
        }

        success = true;
      }
    }

    if (reply) {
      free(reply);
    }

    Kernel::ABI::IPC::Close(replyHandle);

    return success;
  }

  /**
   * @brief Closes an overlay.
   * @param id The overlay resource ID to close.
   */
  inline void CloseOverlay(OverlayResourceID id) {
    using namespace Kernel;

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return;

    CloseOverlayRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::CloseOverlay;
    request.ID = id;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);
  }

  /**
   * @brief Invalidates an overlay's content, triggering a compositor redraw.
   * @param id The overlay resource ID.
   */
  inline void InvalidateOverlay(OverlayResourceID id) {
    using namespace Kernel;

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return;

    InvalidateOverlayRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::InvalidateOverlay;
    request.ID = id;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);
  }

  /**
   * @brief Sets the position of an overlay.
   * @param id The overlay resource ID.
   * @param x The new x-coordinate.
   * @param y The new y-coordinate.
   */
  inline void SetOverlayPosition(OverlayResourceID id, Int16 x, Int16 y) {
    using namespace Kernel;

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return;

    SetOverlayPositionRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::SetOverlayPosition;
    request.ID = id;
    request.X = x;
    request.Y = y;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);
  }

  /**
   * @brief Creates a new window on the application server.
   * @param x The x-coordinate of the window's top-left corner.
   * @param y The y-coordinate of the window's top-left corner.
   * @param width The width of the window in pixels.
   * @param height The height of the window in pixels.
   * @param title The null-terminated title string.
   * @param outID Pointer to receive the assigned window resource ID.
   * @param outResult Optional pointer to receive the full creation result.
   * @param contentColor The content area background color.
   * @param maximized If true, the window is created maximized.
   * @param allowClose If true, the window has a close button.
   * @param allowMaximize If true, the window has a maximize button.
   * @return `true` if the window was created successfully; `false` otherwise.
   */
  inline bool CreateWindow(
    Int16 x,
    Int16 y,
    UInt16 width,
    UInt16 height,
    const char* title,
    WindowResourceID* outID,
    CreateWindowResult* outResult = nullptr,
    UInt32 contentColor = 0xFFFFFFFF,
    bool maximized = false,
    bool allowClose = true,
    bool allowMaximize = true,
    bool allowResize = true,
    UInt32 innerContentColor = 0,
    bool chromeless = false,
    bool noFocus = false
  ) {
    using namespace Kernel;

    IPCPortID replyPortID = static_cast<IPCPortID>(
      400 + Kernel::ABI::Process::GetID() * 3
    );

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::Log::Write(
        Quantum::Core::LogLevel::Trace,
        "CreateWindow failed to open send to port %u",
        static_cast<UInt32>(PortID)
      );

      return false;
    }

    IPCPortResourceID replyHandle = Kernel::ABI::IPC::Open(
      replyPortID,
      IPCPortRights::Manage | IPCPortRights::Receive
    );

    if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::Log::Write(
        Quantum::Core::LogLevel::Trace,
        "CreateWindow failed to open reply port %u",
        static_cast<UInt32>(replyPortID)
      );

      Kernel::ABI::IPC::Close(sendHandle);

      return false;
    }

    CreateWindowRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::CreateWindow;
    request.ReplyPortID = replyPortID;
    request.X = x;
    request.Y = y;
    request.Width = width;
    request.Height = height;

    Size i = 0;

    if (title)
      for (; title[i] && i < 63; ++i)
        request.Title[i] = title[i];

    request.Title[i] = '\0';
    request.ContentColor = contentColor;
    request.InnerContentColor = innerContentColor;
    request.Maximized = maximized;
    request.AllowClose = allowClose;
    request.AllowMaximize = allowMaximize;
    request.AllowResize = allowResize;
    request.Chromeless = chromeless;
    request.NoFocus = noFocus;
    request.CreatorProcessID = Kernel::ABI::Process::GetID();

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);

    IPCMessage* reply = Kernel::ABI::IPC::Receive(replyHandle);

    bool success = false;

    if (
      reply &&
      reply->PayloadSizeInBytes >= sizeof(CreateWindowResult)
    ) {
      auto* result = static_cast<const CreateWindowResult*>(reply->Payload);

      if (result->Success) {
        if (outID) *outID = result->ID;
        if (outResult) *outResult = *result;
        success = true;
      }
    }

    if (reply)
      free(reply);

    Kernel::ABI::IPC::Close(replyHandle);

    return success;
  }

  /**
   * @brief Closes an existing window on the application server.
   * @param id The resource ID of the window to close.
   * @return `true` if the request was sent successfully; `false` otherwise.
   */
  inline bool CloseWindow(WindowResourceID id) {
    using namespace Kernel;

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return false;

    CloseWindowRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::CloseWindow;
    request.ID = id;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);

    return true;
  }

  /**
   * @brief Sets the position of an existing window.
   * @param id The resource ID of the window to reposition.
   * @param x The new x-coordinate of the window's top-left corner.
   * @param y The new y-coordinate of the window's top-left corner.
   * @return `true` if the request was sent successfully; `false` otherwise.
   */
  inline bool SetPosition(WindowResourceID id, Int16 x, Int16 y) {
    using namespace Kernel;

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return false;

    SetPositionRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::SetPosition;
    request.ID = id;
    request.X = x;
    request.Y = y;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);

    return true;
  }

  /**
   * @brief Sets the size of an existing window.
   * @param id The resource ID of the window to resize.
   * @param width The new width of the window in pixels.
   * @param height The new height of the window in pixels.
   * @return `true` if the request was sent successfully; `false` otherwise.
   */
  inline bool SetSize(WindowResourceID id, UInt16 width, UInt16 height) {
    using namespace Kernel;

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return false;

    SetSizeRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::SetSize;
    request.ID = id;
    request.Width = width;
    request.Height = height;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);

    return true;
  }

  /**
   * @brief Blocks until the window's close button is clicked.
   *
   * Sends a `WaitForClose` request to the Application Server and blocks
   * on the reply port. The server responds when the user clicks the
   * close button, allowing the caller to clean up before exiting.
   *
   * @param id The resource ID of the window to wait on.
   */
  inline void WaitForClose(WindowResourceID id) {
    using namespace Kernel;

    IPCPortID replyPortID = static_cast<IPCPortID>(
      401 + Kernel::ABI::Process::GetID() * 3
    );

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return;

    IPCPortResourceID replyHandle = Kernel::ABI::IPC::Open(
      replyPortID,
      IPCPortRights::Manage | IPCPortRights::Receive
    );

    if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::IPC::Close(sendHandle);

      return;
    }

    WaitForCloseRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::WaitForClose;
    request.ReplyPortID = replyPortID;
    request.ID = id;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);

    // block until the server replies (close button clicked)
    IPCMessage* reply = Kernel::ABI::IPC::Receive(replyHandle);

    if (reply) {
      free(reply);
    }

    Kernel::ABI::IPC::Close(replyHandle);
  }

  /**
   * @brief Marks a window's content area as dirty, triggering a compositor
   *        redraw.
   * @param id The resource ID of the window to invalidate.
   * @param dirtyX Content-relative X offset of the dirty region.
   * @param dirtyY Content-relative Y offset of the dirty region.
   * @param dirtyWidth Width of the dirty region (0 = full width).
   * @param dirtyHeight Height of the dirty region (0 = full height).
   */
  inline void InvalidateContent(
    WindowResourceID id,
    UInt16 dirtyX = 0,
    UInt16 dirtyY = 0,
    UInt16 dirtyWidth = 0,
    UInt16 dirtyHeight = 0,
    SharedBufferID contentBufferID = 0,
    UInt16 contentWidth = 0,
    UInt16 contentHeight = 0,
    UInt16 contentStride = 0,
    UInt8 contentBytesPerPixel = 0
  ) {
    using namespace Kernel;

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return;

    InvalidateContentRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::InvalidateContent;
    request.ID = id;
    request.DirtyX = dirtyX;
    request.DirtyY = dirtyY;
    request.DirtyWidth = dirtyWidth;
    request.DirtyHeight = dirtyHeight;
    request.ContentBufferID = contentBufferID;
    request.ContentWidth = contentWidth;
    request.ContentHeight = contentHeight;
    request.ContentStride = contentStride;
    request.ContentBytesPerPixel = contentBytesPerPixel;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);
  }

  /**
   * @brief Blocks until a window event is available for the given window.
   *
   * The Application Server delivers keyboard events (forwarded to the
   * focused window) and resize notifications through this single call.
   *
   * @param id The resource ID of the window.
   * @param outResult Pointer to receive the event result.
   * @return `true` if an event was received; `false` on failure.
   */
  inline bool GetWindowEvent(
    WindowResourceID id, WindowEventResult* outResult
  ) {
    using namespace Kernel;

    IPCPortID replyPortID = static_cast<IPCPortID>(
      402 + Kernel::ABI::Process::GetID() * 3
    );

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return false;

    IPCPortResourceID replyHandle = Kernel::ABI::IPC::Open(
      replyPortID,
      IPCPortRights::Manage | IPCPortRights::Receive
    );

    if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::IPC::Close(sendHandle);

      return false;
    }

    GetWindowEventRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::GetWindowEvent;
    request.ReplyPortID = replyPortID;
    request.ID = id;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);

    IPCMessage* reply = Kernel::ABI::IPC::Receive(replyHandle);

    bool success = false;

    if (
      reply &&
      reply->PayloadSizeInBytes >= sizeof(WindowEventResult)
    ) {
      auto* result = static_cast<const WindowEventResult*>(
        reply->Payload
      );

      if (result->HasEvent && outResult)
        *outResult = *result;

      success = result->HasEvent;
    }

    if (reply) {
      free(reply);
    }

    Kernel::ABI::IPC::Close(replyHandle);

    return success;
  }

  /**
   * @brief Non-blocking variant of @ref GetWindowEvent. Returns
   *        immediately with `false` when no events are queued, instead
   *        of blocking until one arrives.
   *
   * @param id The resource ID of the window.
   * @param outResult Pointer to receive the event result.
   * @return `true` if an event was received; `false` if no event is
   *         pending or on failure.
   */
  inline bool TryGetWindowEvent(
    WindowResourceID id, WindowEventResult* outResult
  ) {
    using namespace Kernel;

    IPCPortID replyPortID = static_cast<IPCPortID>(
      402 + Kernel::ABI::Process::GetID() * 3
    );

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return false;

    IPCPortResourceID replyHandle = Kernel::ABI::IPC::Open(
      replyPortID,
      IPCPortRights::Manage | IPCPortRights::Receive
    );

    if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::IPC::Close(sendHandle);

      return false;
    }

    GetWindowEventRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::TryGetWindowEvent;
    request.ReplyPortID = replyPortID;
    request.ID = id;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);

    IPCMessage* reply = Kernel::ABI::IPC::Receive(replyHandle);

    bool success = false;

    if (
      reply &&
      reply->PayloadSizeInBytes >= sizeof(WindowEventResult)
    ) {
      auto* result = static_cast<const WindowEventResult*>(
        reply->Payload
      );

      if (result->HasEvent && outResult)
        *outResult = *result;

      success = result->HasEvent;
    }

    if (reply) {
      free(reply);
    }

    Kernel::ABI::IPC::Close(replyHandle);

    return success;
  }

  /**
   * @brief Sets the minimum allowed size for a window.
   * @param id The resource ID of the window.
   * @param minWidth The minimum width in pixels.
   * @param minHeight The minimum height in pixels.
   */
  inline void SetMinimumSize(
    WindowResourceID id, UInt16 minWidth, UInt16 minHeight
  ) {
    using namespace Kernel;

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return;

    SetMinimumSizeRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::SetMinimumSize;
    request.ID = id;
    request.MinWidth = minWidth;
    request.MinHeight = minHeight;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);
  }

  /**
   * @brief Invalidates a window's content area with a pixel-shift hint.
   * @param id The resource ID of the window.
   * @param shiftDeltaY Signed pixel shift (positive = up, negative = down).
   * @param dirtyX Content-relative X offset of the dirty strip.
   * @param dirtyY Content-relative Y offset of the dirty strip.
   * @param dirtyWidth Width of the dirty strip.
   * @param dirtyHeight Height of the dirty strip.
   */
  inline void InvalidateContentShift(
    WindowResourceID id,
    Int16 shiftDeltaY,
    UInt16 dirtyX,
    UInt16 dirtyY,
    UInt16 dirtyWidth,
    UInt16 dirtyHeight
  ) {
    using namespace Kernel;

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return;

    InvalidateContentShiftRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::InvalidateContentShift;
    request.ID = id;
    request.ShiftDeltaY = shiftDeltaY;
    request.DirtyX = dirtyX;
    request.DirtyY = dirtyY;
    request.DirtyWidth = dirtyWidth;
    request.DirtyHeight = dirtyHeight;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);
  }

  /**
   * @brief Sets a modal relationship between a dialog window and its parent.
   *        While modal, clicks on the parent are blocked and focus is
   *        redirected to the dialog.
   * @param dialogID The resource ID of the dialog window.
   * @param parentID The resource ID of the parent window to block, or `0` for
   *                 the active window.
   */
  inline void SetModal(WindowResourceID dialogID, WindowResourceID parentID) {
    using namespace Kernel;

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return;

    SetModalRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::SetModal;
    request.DialogID = dialogID;
    request.ParentID = parentID;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);
  }

  /**
   * @brief Delivers a menu action event to a window on the application
   *        server. Used by the context server to route invoked actions.
   * @param id The resource ID of the target window.
   * @param actionID The action ID to deliver.
   */
  inline void DeliverMenuAction(WindowResourceID id, UInt32 actionID) {
    using namespace Kernel;

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return;

    DeliverMenuActionRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::DeliverMenuAction;
    request.ID = id;
    request.ActionID = actionID;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);
  }

  // -----------------------------------------------------------------------
  // GetSystemFonts
  // -----------------------------------------------------------------------

  /**
   * @brief Describes a single system font entry in the
   *        @ref GetSystemFontsResult.
   */
  struct SystemFontEntry {
    /**
     * @brief Shared buffer ID containing the raw font file data (QBF or
     *        PSF format). `0` means no font is available for this role.
     */
    SharedBufferID BufferID;

    /**
     * @brief Size of the font data in the shared buffer, in bytes.
     */
    UInt32 DataSize;
  };

  /**
   * @brief Request to retrieve the system font table.
   */
  struct GetSystemFontsRequest
    : public ABIRequestWithReplyPort<Operation> {};

  /**
   * @brief Result of @ref Operation::GetSystemFonts.
   *
   * Contains shared buffer references for each system font role. The
   * client attaches the buffer, parses the font data (QBF or PSF), and
   * uses the resulting @ref Fonts::BitmapFont.
   *
   * Font roles are accessed by index. Currently defined roles:
   *   - Index 0: **Default**, the primary UI font used for labels,
   *     buttons, text input, and terminal text.
   */
  struct GetSystemFontsResult {
    /**
     * @brief Whether the query was successful.
     */
    bool Success;

    /**
     * @brief Maximum number of font entries in the result.
     */
    static constexpr Size MaxFonts = 4;

    /**
     * @brief Number of valid entries in @ref Fonts.
     */
    Size FontCount;

    /**
     * @brief Font entries indexed by role.
     */
    SystemFontEntry Fonts[MaxFonts];
  };

  /**
   * @brief Queries the system font table from the application server.
   * @param outResult Pointer to receive the font table.
   * @return `true` if the query was successful; `false` otherwise.
   */
  inline bool GetSystemFonts(GetSystemFontsResult* outResult) {
    using Kernel::IPC::IPCPortResourceID;
    using Kernel::IPC::IPCPortID;
    using Kernel::IPC::IPCPortRights;

    IPCPortResourceID sendHandle
      = Kernel::ABI::IPC::Open(PortID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return false;

    IPCPortID replyPortID = static_cast<IPCPortID>(-1);
    IPCPortResourceID replyHandle = Kernel::ABI::IPC::Open(
      static_cast<IPCPortID>(-1),
      IPCPortRights::Manage | IPCPortRights::Receive,
      &replyPortID
    );

    if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::IPC::Close(sendHandle);

      return false;
    }

    GetSystemFontsRequest request;

    request.ABIVersion = Version;
    request.Operation = Operation::GetSystemFonts;
    request.ReplyPortID = replyPortID;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);

    Kernel::IPC::IPCMessage* reply
      = Kernel::ABI::IPC::Receive(replyHandle);

    bool success = false;

    if (
      reply &&
      reply->PayloadSizeInBytes >= sizeof(GetSystemFontsResult)
    ) {
      auto* result = static_cast<const GetSystemFontsResult*>(
        reply->Payload
      );

      if (result->Success && outResult) {
        *outResult = *result;
        success = true;
      }
    }

    if (reply) {
      free(reply);
    }

    Kernel::ABI::IPC::Close(replyHandle);

    return success;
  }
}
