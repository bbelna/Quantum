/**
 * @file System/Kernel/Main.cpp
 * @brief Kernel main entry point.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Types.hpp>

#include "Arch/Paging.hpp"
#include "BootInfo.hpp"
#include "Devices/DeviceManager.hpp"
#include "InitBundle.hpp"
#include "Interrupts.hpp"
#include "Main.hpp"
#include "Prelude.hpp"
#include "Runtime.hpp"
#include "Task.hpp"
#include "TestRunner.hpp"

namespace Quantum::System::Kernel {
  void Main(UInt32 bootInfoPhysicalAddress) {
    using Arch::Paging;
    using Devices::DeviceManager;

    BootInfo::Initialize(bootInfoPhysicalAddress);
    Paging::Initialize(bootInfoPhysicalAddress);
    Interrupts::Initialize();

    RunGlobalConstructors();

    DeviceManager::Initialize();
    InitBundle::Initialize();
    Task::Initialize();

    #if defined(KERNEL_TESTS)
    Task::Create(TestRunner::Run, 4096);
    #else
    Task::Create(InitBundle::LaunchCoordinatorTask, 4096);
    #endif

    Task::Yield();
  }
}
