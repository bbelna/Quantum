/**
 * @file Drivers/Input/PS2Mouse/PS2MouseDriverTypes.hpp
 * @brief Declares core @ref @QDrvs::Input::PS2Mouse types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Input.hpp>
#include <Quantum/OS.hpp>
#include <Quantum/Threading.hpp>

namespace Quantum::Drivers::Input::PS2Mouse {
  namespace KernelLog = Quantum::Kernel::ABI::Log;
  namespace KernelIPC = Quantum::Kernel::ABI::IPC;
  namespace Threads = Quantum::Kernel::ABI::Thread;
  namespace Interrupts = Quantum::Kernel::ABI::Interrupt;
  namespace Process = Quantum::Kernel::ABI::Process;
  namespace PortIO = Quantum::Kernel::ABI::PortIO;
}

using namespace Quantum::Core;
using namespace Quantum::Input;
using namespace Quantum::Threading;

using namespace Quantum::Clients;
using namespace Quantum::HAL;
using namespace Quantum::Kernel;
using namespace Quantum::Kernel::Resources;
using namespace Quantum::Servers::Input::ABI;
using namespace Quantum::Servers::Startup::ABI;
using namespace Quantum::Drivers::Input::PS2Mouse;
