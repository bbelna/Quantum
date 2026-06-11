/**
 * @file Servers/App/Overlays/Overlay.cpp
 * @brief Implements @ref @QAppSrv::Overlays::Overlay.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "Overlay.hpp"

namespace Quantum::Servers::App::Overlays {
  bool Overlay::DeliverEvent(
    const ABI::WindowEventResult& event,
    KernelClient& kernel
  ) {
    if (EventReplyPortID != 0) {
      IPCPortResourceID replyHandle = kernel.OpenIPCPort(
        EventReplyPortID,
        IPCPortRights::Send
      );

      if (replyHandle != static_cast<IPCPortResourceID>(-1)) {
        kernel.SendIPCMessage(replyHandle, &event, sizeof(event));
        kernel.CloseIPCPort(replyHandle);
      }

      EventReplyPortID = 0;

      return true;
    }

    if (EventQueueCount < EventQueueCapacity) {
      Size tail = (EventQueueHead + EventQueueCount) % EventQueueCapacity;

      EventQueue[tail] = event;
      EventQueueCount++;

      return true;
    }

    return false;
  }
}
