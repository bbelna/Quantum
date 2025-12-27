/**
 * @file System/Kernel/Main.cpp
 * @brief Kernel main entry point.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <Types.hpp>

#include "BootInfo.hpp"
#include "DeviceManager.hpp"
#include "InitBundle.hpp"
#include "Interrupts.hpp"
#include "Main.hpp"
#include "Memory.hpp"
#include "Prelude.hpp"
#include "Task.hpp"
#include "TestRunner.hpp"

namespace Quantum::System::Kernel {
  void Main(UInt32 bootInfoPhysicalAddress) {
    BootInfo::Initialize(bootInfoPhysicalAddress);
    Memory::Initialize(bootInfoPhysicalAddress);
    Interrupts::Initialize();
    DeviceManager::Initialize();
    Task::Initialize();
    InitBundle::Initialize();

    #ifdef KERNEL_TESTS
    // spawn test runner task and start scheduling
    Task::Create(TestRunner::Run, 4096);
    #else
    Task::Create(InitBundle::LaunchCoordinatorTask, 4096);
    #endif

    // enter scheduler; if no tests are queued we fall back to idle
    Task::Yield();
  }
}
