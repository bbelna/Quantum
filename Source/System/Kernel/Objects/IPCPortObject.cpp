/**
 * @file System/Kernel/Objects/IPCPortObject.cpp
 * @brief IPC port kernel object.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Objects/IPCPortObject.hpp"

namespace Quantum::System::Kernel::Objects {
  IPCPortObject::IPCPortObject(UInt32 port)
    : KernelObject(KernelObjectType::IPCPort),
    portId(port)
  {}
}
