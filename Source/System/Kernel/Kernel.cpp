/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Kernel.cpp
 * Core kernel implementation.
 */

#include <BootInfo.hpp>
#include <CPU.hpp>
#include <Handlers/PanicHandler.hpp>
#include <Handlers/SystemCallHandler.hpp>
#include <InitBundle.hpp>
#include <Interrupts.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Memory.hpp>
#include <Task.hpp>
#include <Testing.hpp>
#include <Types.hpp>

namespace Quantum::System::Kernel {
  using LogLevel = Logger::Level;

  namespace {
    #ifdef KERNEL_TESTS
    /**
     * Kernel test runner task entry point.
     */
    void KernelTestRunner() {
      Testing::RegisterBuiltins();
      Testing::RunAll();
      Logger::Write(LogLevel::Info, "Kernel tests task finished");
      Task::Exit();
    }
    #endif
  }

  void Initialize(UInt32 bootInfoPhysicalAddress) {
    BootInfo::Initialize(bootInfoPhysicalAddress);
    Memory::Initialize(bootInfoPhysicalAddress);
    Interrupts::Initialize();
    Task::Initialize();
    Task::EnablePreemption();
    InitBundle::Initialize();

    #ifdef KERNEL_TESTS
    // spawn test runner task and start scheduling
    Task::Create(KernelTestRunner, 4096);
    #else
    Task::Create(InitBundle::LaunchCoordinatorTask, 4096);
    #endif

    // enter scheduler; if no tests are queued we fall back to idle
    Task::Yield();
  }

  Interrupts::Context* HandleSystemCall(Interrupts::Context& context) {
    return Handlers::SystemCallHandler::Handle(context);
  }

  bool GetInitBundleInfo(UInt32& base, UInt32& size) {
    return InitBundle::GetInfo(base, size);
  }

  UInt32 SpawnInitBundleTask(CString name) {
    return InitBundle::SpawnTask(name);
  }

  void Panic(
    String message,
    String file,
    UInt32 line,
    String function
  ) {
    Handlers::PanicHandler::Handle(
      message,
      file,
      line,
      function
    );
  }
}

