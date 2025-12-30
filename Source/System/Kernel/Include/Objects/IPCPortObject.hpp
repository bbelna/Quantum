/**
 * @file System/Kernel/Include/Objects/IPCPortObject.hpp
 * @brief IPC port kernel object.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

#include "Objects/KernelObject.hpp"

namespace Quantum::System::Kernel::Objects {
  /**
   * IPC port kernel object.
   */
  class IPCPortObject : public KernelObject {
    public:
      /**
       * Constructs an IPC port object.
       * @param port
       *   IPC port identifier.
       */
      explicit IPCPortObject(UInt32 port);

      /**
       * IPC port identifier.
       */
      UInt32 portId;
  };
}
