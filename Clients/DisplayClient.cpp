/**
 * @file Clients/DisplayClient.cpp
 * @brief Implements @ref @QClients::DisplayClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ClientsTypes.hpp>
#include "DisplayClient.hpp"
#include "KernelClient.hpp"

namespace Quantum::Clients {
  bool DisplayClient::Initialize() {
    PointerList<Device> devices = _kernel.GetDevices();

    for (Size index = 0; index < devices.GetCount(); ++index) {
      Device& device = devices[index];

      if (device.CategoryID == Enum::ToBase(DeviceCategoryType::Graphics)) {
        _deviceID = device.ID;

        return true;
      }
    }

    return false;
  }

  bool DisplayClient::AcquireDisplay() {
    if (_deviceID == 0) return false;

    AcquireDisplayPayload payload;

    payload.OwnerPID = _kernel.GetProcessID();

    UInt32 result = _kernel.InvokeDriver(
      _deviceID,
      Enum::ToBase(GraphicsDriverOperation::AcquireDisplay),
      &payload
    );

    _acquired = (result == 0);

    return _acquired;
  }

  void DisplayClient::ReleaseDisplay() {
    if (_deviceID == 0 || !_acquired) return;

    ReleaseDisplayPayload payload;

    payload.OwnerPID = _kernel.GetProcessID();

    _kernel.InvokeDriver(
      _deviceID,
      Enum::ToBase(GraphicsDriverOperation::ReleaseDisplay),
      &payload
    );

    _acquired = false;
  }

  bool DisplayClient::IsCompositing() {
    if (_deviceID == 0) return false;

    DisplayOwnerPayload payload = {};

    _kernel.InvokeDriver(
      _deviceID,
      Enum::ToBase(GraphicsDriverOperation::GetDisplayOwner),
      &payload
    );

    return payload.IsCompositing;
  }

  void DisplayClient::WriteText(const char* text) {
    if (_deviceID == 0) return;

    _kernel.InvokeDriver(
      _deviceID,
      Enum::ToBase(GraphicsDriverOperation::WriteText),
      const_cast<void*>(static_cast<const void*>(text))
    );
  }

  void DisplayClient::SetTextForegroundColor(UInt32 color) {
    if (_deviceID == 0) return;

    // WriteText is operation 1 and takes text directly; for
    // SetTextForegroundColor there is no dedicated GraphicsDriverOperation
    // yet, so we skip for now -- text renders in the driver's default color
    (void)color;
  }

  Point DisplayClient::GetTextCursorPosition() {
    // no dedicated operation exists in GraphicsDriverOperation for cursor
    // position query from user space
    return { 0, 0 };
  }

  void DisplayClient::SetTextCursorPosition(Point position) {
    (void)position;
  }

  bool DisplayClient::GetModeInfo(
    UInt16* outWidth,
    UInt16* outHeight,
    UInt8* outBPP
  ) {
    if (_deviceID == 0) return false;

    ModeInfoPayload payload = {};

    _kernel.InvokeDriver(
      _deviceID,
      Enum::ToBase(GraphicsDriverOperation::GetModeInfo),
      &payload
    );

    if (outWidth) *outWidth = payload.Width;
    if (outHeight) *outHeight = payload.Height;
    if (outBPP) *outBPP = payload.BitsPerPixel;

    return true;
  }
}
