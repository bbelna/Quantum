/**
 * @file Servers/Graphics/GraphicsServer.cpp
 * @brief Implements @ref @QGfxSrv::GraphicsServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "GraphicsServer.hpp"

namespace Quantum::Servers::Graphics {
  GraphicsServer::GraphicsServer(
    StartupClient& startup,
    DrawingController& drawingController,
    CursorController& cursorController,
    DisplayController& displayController
  ) :
    Server(GraphicsPortID)
  {
    RegisterController(
      GraphicsServerOperation::WriteText,
      drawingController
    );
    RegisterController(
      GraphicsServerOperation::FillRectangle,
      drawingController
    );
    RegisterController(
      GraphicsServerOperation::BlitBuffer,
      drawingController
    );
    RegisterController(
      GraphicsServerOperation::XORRectangle,
      drawingController
    );
    RegisterController(
      GraphicsServerOperation::DrawText,
      drawingController
    );

    RegisterController(
      GraphicsServerOperation::SetCursorBitmap,
      cursorController
    );
    RegisterController(
      GraphicsServerOperation::MoveCursor,
      cursorController
    );
    RegisterController(
      GraphicsServerOperation::ShowCursor,
      cursorController
    );

    RegisterController(
      GraphicsServerOperation::SetMode,
      displayController
    );
    RegisterController(
      GraphicsServerOperation::GetModeInfo,
      displayController
    );
    RegisterController(
      GraphicsServerOperation::GetBackBuffer,
      displayController
    );
    RegisterController(
      GraphicsServerOperation::ScreenBlit,
      displayController
    );
    RegisterController(
      GraphicsServerOperation::FlushBackBuffer,
      displayController
    );
    RegisterController(
      GraphicsServerOperation::BeginBatch,
      displayController
    );
    RegisterController(
      GraphicsServerOperation::EndBatch,
      displayController
    );

    startup.Ready();
  }
}

/**
 * @brief Entry point for @ref @QGfxSrv::GraphicsServer.
 * @return Non-zero error code on failure. Does **not** return on success.
 * @note This is a system server and should not exit under normal operation.
 *       Returning from this function indicates a failure.
 *
 * Discovers the graphics device, initializes the display, and enters the
 * server loop (@ref @QGfxSrv::GraphicsServer::Run) to handle graphics
 * requests.
 */
int Main() {
  KernelClient kernel;
  StartupClient startup;
  ServerLog log(kernel);
  DeviceClient deviceClient;

  PointerList<Device> gfxDevices
    = deviceClient.GetDevicesInCategory(
        ToDeviceCategoryID(
          DeviceCategoryType::Graphics
        )
      );

  if (gfxDevices.GetCount() == 0) {
    log.Error("No graphics device found");

    return -1;
  }

  Device gfxDevice = gfxDevices[0];

  log.Info(
    "Using %s (%s)",
    gfxDevice.Name,
    gfxDevice.DisplayName
  );

  Cursor cursor;
  Display display(kernel, log, cursor);

  display.driver = new GraphicsDriverClient();
  display.driver->Initialize(gfxDevice.ID);
  display.hasHardwareCursor = display.driver->HasHardwareCursor();
  display.hasFastScreenBlit = display.driver->HasFastScreenBlit();

  if (display.hasHardwareCursor) {
    log.Info("Hardware cursor enabled");
  }

  if (display.hasFastScreenBlit) {
    log.Info("Fast screen BLT enabled");
  }

  // Query the mode the bootloader already set
  {
    auto info = display.driver->QueryModeInfo();

    display.screenWidth = info.Width;
    display.screenHeight = info.Height;
    display.bitsPerPixel = info.BitsPerPixel;
    display.pitch = info.Pitch;

    display.backBuffer.Allocate(
      display.screenWidth,
      display.screenHeight,
      display.bitsPerPixel
    );

    display.framebufferBufferID
      = display.driver->GetFramebufferBufferID();
    display.hasDirectFramebuffer
      = (display.framebufferBufferID != 0)
      && display.hasHardwareCursor;

    log.Trace(
      "%ux%u %ubpp (pitch %u)",
      static_cast<UInt32>(display.screenWidth),
      static_cast<UInt32>(display.screenHeight),
      static_cast<UInt32>(display.bitsPerPixel),
      static_cast<UInt32>(display.pitch)
    );

    if (display.hasDirectFramebuffer) {
      log.Info(
        "Direct framebuffer mode enabled with buffer ID %u",
        display.framebufferBufferID
      );
    }
  }

  constexpr UInt32 GraphicsServerPriority = 20;

  kernel.SetThreadPriority(GraphicsServerPriority);

  DrawingController drawingController(kernel, log, display);
  CursorController cursorController(kernel, log, display, cursor);
  DisplayController displayController(kernel, log, display, cursor);

  GraphicsServer server(
    startup,
    drawingController,
    cursorController,
    displayController
  );

  return server.Run();
}
