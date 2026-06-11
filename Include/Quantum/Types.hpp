/**
 * @file Include/Quantum/Types.hpp
 * @brief Centralized type definitions for QuantumOS.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Threading.hpp>

#include "Kernel/Concurrency.hpp"
#include "Kernel/IPC.hpp"
#include "Kernel/Memory.hpp"

namespace QOSKernel = Quantum::Kernel;
namespace QThreading = Quantum::Threading;

inline constexpr auto InvalidIPCPortResourceID
  = QOSKernel::IPC::InvalidIPCPortResourceID;

using ProcessID = QOSKernel::Concurrency::ProcessID;
using ProcessInfo = QOSKernel::Concurrency::ProcessInfo;
using ProcessSpawnParameters = QOSKernel::Concurrency::ProcessSpawnParameters;
using ProcessSpawnSegment = QOSKernel::Concurrency::ProcessSpawnSegment;
using SharedBufferID = QOSKernel::Memory::SharedBufferID;

using IPCPortID = QOSKernel::IPC::IPCPortID;
using IPCPortResourceID = QOSKernel::IPC::IPCPortResourceID;
using IPCPortHandle = IPCPortResourceID;
using IPCPortRights = QOSKernel::IPC::IPCPortRights;
using IPCMessage = QOSKernel::IPC::IPCMessage;

using ProcessPermissions = QThreading::ProcessPermissions;
