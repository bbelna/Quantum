/**
 * @file Servers/Graphics/Core/IPC/ReplyPortCache.cpp
 * @brief Implements @ref @QGfxSrv::ReplyPortCache.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "ReplyPortCache.hpp"

namespace Quantum::Servers::Graphics {
  ReplyPortCache::ReplyPortCache(KernelClient& kernel)
    : _kernel(kernel)
  {
  }

  void ReplyPortCache::Send(
    UInt16 replyPortID,
    const void* payload,
    Size payloadSizeInBytes
  ) {
    if (replyPortID == 0) {
      return;
    }

    if (
      _portID != replyPortID ||
      _handle == static_cast<IPCPortResourceID>(-1)
    ) {
      if (_handle != static_cast<IPCPortResourceID>(-1)) {
        _kernel.CloseIPCPort(_handle);
      }

      _handle = _kernel.OpenIPCPort(
        replyPortID, IPCPortRights::Send
      );
      _portID = replyPortID;
    }

    if (_handle != static_cast<IPCPortResourceID>(-1)) {
      _kernel.SendIPCMessage(
        _handle,
        payload,
        payloadSizeInBytes
      );
    }
  }
}
