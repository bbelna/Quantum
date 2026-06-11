/**
 * @file Servers/App/AppServer.cpp
 * @brief Implements @ref @QAppSrv::AppServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "AppServer.hpp"
#include "Controllers/OverlayController.hpp"
#include "Controllers/WindowController.hpp"
#include "Dock/Dock.hpp"

namespace Quantum::Servers::App {
  AppServer::AppServer(
    ServerLog& log
  ) :
    Server(App::ABI::PortID),
    _kernel(),
    _log(log),
    _windowManager(
      _kernel,
      log
    )
  {
  }

  void AppServer::Start() {
    IPCPortResourceID gfxHandle;

    // wait for the graphics server, then open send handle
    _kernel.WaitForIPCPort(GraphicsPortID);

    gfxHandle = _kernel.OpenIPCPort(
      GraphicsPortID,
      IPCPortRights::Send
    );

    IPCPortID gfxFlushReplyPortID = INVALID_IPC_PORT_ID;
    IPCPortResourceID gfxFlushReplyHandle = _kernel.OpenIPCPort(
      INVALID_IPC_PORT_ID,
      IPCPortRights::Manage | IPCPortRights::Receive,
      &gfxFlushReplyPortID
    );

    GraphicsGetModeInfoResult modeInfo;
    GraphicsGetModeInfoRequest modeInfoRequest;

    modeInfoRequest.ABIVersion = GraphicsABIVersion;
    modeInfoRequest.Operation = GraphicsOperation::GetModeInfo;

    // query display mode
    modeInfo = InvokeOS<
      GraphicsGetModeInfoResult,
      GraphicsGetModeInfoRequest
    >(GraphicsPortID, 0, modeInfoRequest);

    if (modeInfo.Width > 0) {
      _scaleFactor = modeInfo.ScaleFactor > 0
        ? modeInfo.ScaleFactor
        : 1;
      _screenWidth = static_cast<UInt16>(modeInfo.Width / _scaleFactor);
      _screenHeight = static_cast<UInt16>(modeInfo.Height / _scaleFactor);
      _hasHardwareCursor = modeInfo.HardwareCursor;
      _hasFastScreenBlit = modeInfo.HasFastScreenBlit;
    }

    // load fonts from file system
    _loadTitleFont();
    _loadDefaultFont();

    // use the default system font for window title bars
    Window::SetTitleFont(&_defaultFont);

    GraphicsClient graphics(
      gfxHandle,
      gfxFlushReplyHandle,
      gfxFlushReplyPortID,
      _scaleFactor
    );

    _graphics = &graphics;

    CursorManager cursor(graphics, _screenWidth, _screenHeight);

    _cursor = &cursor;

    void* compositeBuffer = nullptr;
    void* sharedBuffer = nullptr;
    UInt8 bufferBPP = 4;
    UInt16 bufferWidth = 0;
    bool directFramebuffer = false;

    // attach shared compositor buffer
    GraphicsGetBackBufferResult bufferInfo;

    GraphicsGetBackBufferRequest backBufferRequest = {};
    backBufferRequest.ABIVersion = GraphicsABIVersion;
    backBufferRequest.Operation = GraphicsOperation::GetBackBuffer;

    bufferInfo = InvokeOS<
      GraphicsGetBackBufferResult,
      GraphicsGetBackBufferRequest
    >(GraphicsPortID, 0, backBufferRequest);

    if (bufferInfo.BufferID != 0) {
      UIntPtr bufferAddress
        = _kernel.AttachSharedBuffer(bufferInfo.BufferID);

      if (bufferAddress != 0) {
        sharedBuffer = reinterpret_cast<void*>(bufferAddress);
        bufferWidth = bufferInfo.Width;
        bufferBPP = (bufferInfo.BitsPerPixel <= 16) ? 2 : 4;
        directFramebuffer = bufferInfo.DirectFramebuffer;

        compositeBuffer = sharedBuffer;

        if (directFramebuffer) {
          Size bufferSize
            = static_cast<Size>(bufferWidth)
            * bufferInfo.Height
            * bufferBPP;
          UIntPtr newCompositeBuffer = AllocateBlock(bufferSize);

          if (newCompositeBuffer != 0) {
            compositeBuffer = reinterpret_cast<void*>(newCompositeBuffer);
          }
        }
      }
    }

    // wire up the GraphicsClient shared state
    GraphicsClientShared bridgeShared;

    bridgeShared.ScreenWidth = &_screenWidth;
    bridgeShared.ScreenHeight = &_screenHeight;
    bridgeShared.CompositeBuffer = &compositeBuffer;
    bridgeShared.SharedBuffer = &sharedBuffer;
    bridgeShared.BufferBpp = &bufferBPP;
    bridgeShared.BufferWidth = &bufferWidth;
    bridgeShared.DirectFramebuffer = &directFramebuffer;
    bridgeShared.RenderSignal = &_compositor.GetRenderSignal();
    bridgeShared.DebugFlushRegion = &_compositor.DebugFlushRegion;

    graphics.SetSharedState(bridgeShared);

    // initialize compositor
    _compositor.Init(
      &graphics,
      &_windowManager,
      &_stateLock,
      &_kernel,
      _screenWidth,
      _screenHeight,
      bufferBPP,
      bufferWidth,
      compositeBuffer,
      sharedBuffer,
      directFramebuffer,
      _hasHardwareCursor
    );

    // create input dispatcher
    InputDispatcher input(
      _kernel,
      graphics,
      _windowManager,
      _compositor,
      cursor,
      _screenWidth,
      _screenHeight,
      _hasHardwareCursor,
      _hasFastScreenBlit,
      bufferBPP
    );

    _input = &input;
    _input->DebugFlushRegion = &_compositor.DebugFlushRegion;

    // create overlay controller
    Controllers::OverlayController overlayController(
      _kernel, _log, _compositor
    );

    _overlayController = &overlayController;

    // create window controller
    Controllers::WindowController windowController(
      _kernel,
      _log,
      _windowManager,
      _compositor,
      graphics,
      overlayController.GetOverlayManager(),
      _screenWidth,
      _screenHeight,
      bufferBPP,
      _hasHardwareCursor,
      _hasFastScreenBlit,
      _primaryFontBufferID,
      _primaryFontDataSizeInBytes,
      _secondaryFontBufferID,
      _secondaryFontDataSizeInBytes
    );

    _windowController = &windowController;

    // register all operations with the server dispatch table
    RegisterController(ABI::Operation::CreateWindow, windowController);
    RegisterController(ABI::Operation::CloseWindow, windowController);
    RegisterController(ABI::Operation::SetPosition, windowController);
    RegisterController(ABI::Operation::SetSize, windowController);
    RegisterController(ABI::Operation::WaitForClose, windowController);
    RegisterController(ABI::Operation::InvalidateContent, windowController);
    RegisterController(ABI::Operation::InvalidateContentShift, windowController);
    RegisterController(ABI::Operation::GetWindowEvent, windowController);
    RegisterController(ABI::Operation::SetMinimumSize, windowController);
    RegisterController(ABI::Operation::SetModal, windowController);
    RegisterController(ABI::Operation::GetSystemFonts, windowController);
    RegisterController(ABI::Operation::DeliverMenuAction, windowController);
    RegisterController(ABI::Operation::TryGetWindowEvent, windowController);

    RegisterController(ABI::Operation::CreateOverlay, overlayController);
    RegisterController(ABI::Operation::CloseOverlay, overlayController);
    RegisterController(ABI::Operation::InvalidateOverlay, overlayController);
    RegisterController(ABI::Operation::GetOverlayEvent, overlayController);
    RegisterController(ABI::Operation::SetOverlayPosition, overlayController);

    _compositor.SetOverlayManager(&overlayController.GetOverlayManager());
    input.SetOverlayManager(&overlayController.GetOverlayManager());

    // create dock
    Dock::Dock dock;

    dock.Init(
      &_windowManager,
      _screenWidth,
      _screenHeight,
      &_defaultFont,
      &_defaultFont
    );

    _compositor.SetDock(&dock);
    input.SetDock(&dock);

    _kernel.SetLogLevel(LogLevel::Critical);

    Rectangle fullScreen(0, 0, _screenWidth, _screenHeight);

    // initial screen setup
    if (compositeBuffer) {
      _compositor.BufferFillRectangle(fullScreen, Compositor::BackgroundColor);
      graphics.FlushBackBuffer(0, 0, _screenWidth, _screenHeight);
    } else {
      graphics.FillRectangle(
        0,
        0,
        _screenWidth,
        _screenHeight,
        Compositor::BackgroundColor
      );
    }

    cursor.Init();

    _compositor.Damage(fullScreen);
    _compositor.RenderImmediate();

    cursor.MoveTo(cursor.GetPosition());
    cursor.Show(true);

    // elevate to real-time priority
    _kernel.SetThreadPriority(
      static_cast<UInt32>(Kernel::ABI::Thread::Priority::DisplayServer)
    );

    // spawn the render thread
    Thread::Create(
      Compositor::RenderThreadEntry,
      reinterpret_cast<UInt32>(&_compositor)
    );

    // main event loop

    IPCPortResourceID waitHandles[2] = {
      input.GetInputReplyHandle(),
      _portHandle
    };

    { Quantum::Clients::StartupClient _s; _s.Ready(); }

    for (;;) {
      // drain pending app requests
      _stateLock.Lock();

      if (_portHandle != static_cast<IPCPortResourceID>(-1)) {
        for (;;) {
          IPCMessage* appMsg
            = _kernel.TryReceiveIPCMessage(_portHandle);

          if (!appMsg) break;

          ProcessNextMessage(appMsg);

          free(appMsg);
        }
      }

      _stateLock.Unlock();

      // signal the render thread
      _compositor.SignalRender();

      // ensure an input request is outstanding
      if (!input.IsInputRequestPending()) input.SendInputRequest();

      // multiplexed wait for input events or app requests
      IPCMessage* msg = nullptr;
      Size which = 0;

      if (input.IsInputRequestPending()) {
        msg = _kernel.TryReceiveIPCMessage(input.GetInputReplyHandle());
      }

      if (!msg) {
        msg = _kernel.TryReceiveIPCMessage(_portHandle);

        if (msg) which = 1;
      }

      if (!msg) {
        msg = _kernel.ReceiveAnyIPCMessage(waitHandles, 2, &which);
      }

      // bail if still no msg
      if (!msg) continue;

      // app request - drain all queued messages before rendering so
      // that closely related requests (e.g. CreateWindow followed by
      // SetModal) are both processed before the dock rebuilds
      if (which == 1) {
        _stateLock.Lock();

        do {
          ProcessNextMessage(msg);

          free(msg);

          msg = _kernel.TryReceiveIPCMessage(_portHandle);
        } while (msg);

        if (windowController.ConsumeDockDirty()) dock.SetDirty();

        _stateLock.Unlock();
        _compositor.SignalRender();

        continue;
      }

      // input event
      input.SetInputRequestPending(false);

      InputEvent event;
      bool gotEvent = false;

      if (
        msg->PayloadSizeInBytes
          >= sizeof(Input::ABI::InputGetNextEventResult)
      ) {
        auto* result = static_cast<const Input::ABI::InputGetNextEventResult*>(
          msg->Payload
        );

        if (result->HasEvent) {
          event = result->Event;
          gotEvent = true;
        }
      }

      free(msg);

      if (!gotEvent) continue;

      // immediately update the cursor position before acquiring the lock
      // so the cursor stays responsive even when the render thread is busy
      if (
        event.Type == InputEventType::MouseMove &&
        !input.IsInteracting()
      ) {
        cursor.UpdatePosition(event.DeltaX, event.DeltaY);
        input.SetCursorPreUpdated();
      }

      _stateLock.Lock();

      input.Dispatch(event);

      _stateLock.Unlock();
      _compositor.SignalRender();
    }
  }

  void AppServer::_loadTitleFont() {
    namespace FSABI = ::Quantum::Servers::FileSystem::ABI;
    Quantum::Clients::FileSystemClient fileSystem;

    const char* path = "QUANTUM/System/Fonts/TitleBar.qbf";

    FSABI::FileSystemFileStat stat = {};

    if (!fileSystem.Stat(path, &stat) || stat.Size == 0) return;

    FSABI::FileHandle handle = fileSystem.Open(
      path,
      static_cast<UInt32>(FSABI::FileSystemOpenFlags::Read)
    );

    if (handle == 0) return;

    _titleFontBuffer = AllocateBlock(stat.Size);

    if (_titleFontBuffer == 0) {
      fileSystem.Close(handle);

      return;
    }

    UInt8* data = reinterpret_cast<UInt8*>(_titleFontBuffer);

    Int32 bytesRead = fileSystem.Read(handle, data, stat.Size, 0);

    fileSystem.Close(handle);

    if (bytesRead <= 0) {
      FreeBlock(_titleFontBuffer);

      _titleFontBuffer = 0;

      return;
    }

    // try QBF first, then PSF
    Size fontDataSize = static_cast<Size>(bytesRead);

    _titleFont = Fonts::QBFParser::Parse(data, fontDataSize);

    if (!_titleFont.GlyphData) {
      _titleFont = Fonts::PSFParser::Parse(data, fontDataSize);
    }

    if (!_titleFont.GlyphData) {
      FreeBlock(_titleFontBuffer);

      _titleFontBuffer = 0;

      return;
    }

    _primaryFontDataSizeInBytes = static_cast<UInt32>(bytesRead);

    // create a shared buffer so client apps can attach the font data
    _primaryFontBufferID
      = _kernel.CreateSharedBuffer(_primaryFontDataSizeInBytes);

    if (_primaryFontBufferID != 0) {
      UIntPtr sharedAddr
        = _kernel.AttachSharedBuffer(_primaryFontBufferID);

      _log.Trace(
        "Title font shared buffer has ID %u and address %p",
        static_cast<UInt32>(_primaryFontBufferID),
        static_cast<UInt32>(sharedAddr)
      );

      if (sharedAddr != 0) {
        Byte::Copy(
          reinterpret_cast<void*>(sharedAddr),
          data,
          _primaryFontDataSizeInBytes
        );

        // intentionally do NOT detach, the shared buffer must remain
        // attached so its AttachCount stays >= 1; otherwise the kernel
        // frees the backing pages when the count drops to 0
      }
    }

    // title font is loaded and shared with clients but the title bar
    // itself uses the default system font (set after _loadDefaultFont)
  }

  void AppServer::_loadDefaultFont() {
    namespace FSABI = ::Quantum::Servers::FileSystem::ABI;
    Quantum::Clients::FileSystemClient fileSystem;

    const char* path = "QUANTUM/System/Fonts/Default.qbf";

    FSABI::FileSystemFileStat stat = {};

    if (!fileSystem.Stat(path, &stat) || stat.Size == 0) return;

    FSABI::FileHandle handle = fileSystem.Open(
      path,
      static_cast<UInt32>(FSABI::FileSystemOpenFlags::Read)
    );

    if (handle == 0) return;

    _defaultFontBuffer = AllocateBlock(stat.Size);

    if (_defaultFontBuffer == 0) {
      fileSystem.Close(handle);

      return;
    }

    auto* data = reinterpret_cast<UInt8*>(_defaultFontBuffer);

    Int32 bytesRead = fileSystem.Read(handle, data, stat.Size, 0);

    fileSystem.Close(handle);

    if (bytesRead <= 0) {
      FreeBlock(_defaultFontBuffer);

      _defaultFontBuffer = 0;

      return;
    }

    // try QBF first, then PSF
    Size defaultFontDataSize = static_cast<Size>(bytesRead);

    _defaultFont = Fonts::QBFParser::Parse(data, defaultFontDataSize);

    if (!_defaultFont.GlyphData) {
      _defaultFont = Fonts::PSFParser::Parse(data, defaultFontDataSize);
    }

    if (!_defaultFont.GlyphData) {
      FreeBlock(_defaultFontBuffer);

      _defaultFontBuffer = 0;

      return;
    }

    _secondaryFontDataSizeInBytes = static_cast<UInt32>(bytesRead);
    _secondaryFontBufferID
      = _kernel.CreateSharedBuffer(_secondaryFontDataSizeInBytes);

    if (_secondaryFontBufferID != 0) {
      UIntPtr sharedAddr
        = _kernel.AttachSharedBuffer(_secondaryFontBufferID);

      _log.Trace(
        "Default font shared buffer has ID %u and address %p",
        static_cast<UInt32>(_secondaryFontBufferID),
        static_cast<UInt32>(sharedAddr)
      );

      if (sharedAddr != 0) {
        Byte::Copy(
          reinterpret_cast<void*>(sharedAddr),
          data,
          _secondaryFontDataSizeInBytes
        );
      }
    }
  }
}

/**
 * @brief Application server process entry point.
 * @return `-1` on failure (should never return under normal operation).
 */
int Main() {
  KernelClient kernel;
  ServerLog log(kernel);
  AppServer* server = new AppServer(log);

  server->Start();

  return -1;
}
