/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Kernel.cpp
 * Core kernel implementation.
 */

#include <CPU.hpp>
#include <Interrupts.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Memory.hpp>
#include <Task.hpp>
#include <Testing.hpp>
#include <Handlers/PanicHandler.hpp>
#include <Handlers/SystemCallHandler.hpp>
#include <Helpers/CStringHelper.hpp>
#include <Helpers/DebugHelper.hpp>
#include <Types/Primitives.hpp>
#include <Types/Logging/LogLevel.hpp>

namespace Quantum::System::Kernel {
  using Helpers::CStringHelper;
  using Types::Logging::LogLevel;

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
    Memory::Initialize(bootInfoPhysicalAddress);
    Interrupts::Initialize();
    Task::Initialize();
    Task::EnablePreemption();

    #ifdef KERNEL_TESTS
    // spawn test runner task and start scheduling
    Task::Create(KernelTestRunner, 4096);
    #endif

    // enter scheduler; if no tests are queued we fall back to idle
    Task::Yield();
  }

  InterruptContext* HandleSystemCall(InterruptContext& context) {
    return Handlers::SystemCallHandler::Handle(context);
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
