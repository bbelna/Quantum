/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Main.cpp
 * Kernel main entry point.
 */

#include "BootInfo.hpp"
#include "CPU.hpp"
#include "DeviceManager.hpp"
#include "Handlers/PanicHandler.hpp"
#include "Handlers/SystemCallHandler.hpp"
#include "InitBundle.hpp"
#include "Interrupts.hpp"
#include "Logger.hpp"
#include "Main.hpp"
#include "Memory.hpp"
#include "Prelude.hpp"
#include "Task.hpp"
#include "Testing.hpp"
#include "TestRunner.hpp"
#include "Types.hpp"

void Main(UInt32 bootInfoPhysicalAddress) {
  using namespace Kernel;

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
