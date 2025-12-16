/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Kernel.cpp
 * Core kernel implementation.
 */

#include <CPU.hpp>
#include <Helpers/CStringHelper.hpp>
#include <Helpers/DebugHelper.hpp>
#include <Interrupts.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Memory.hpp>
#include <Task.hpp>
#include <Tests.hpp>
#include <Types/Logging/Level.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel {
  using CStringHelper = Helpers::CStringHelper;
  using LogLevel = Types::Logging::Level;

  namespace {
    /**
     * Kernel test runner task entry point.
     */
    void KernelTestRunner() {
      Tests::RegisterBuiltins();
      Tests::RunAll();
      Logger::Write(LogLevel::Info, "Kernel tests task finished");
      Task::Exit();
    }
  }

  void Kernel::Initialize(UInt32 bootInfoPhysicalAddress) {
    Memory::Initialize(bootInfoPhysicalAddress);
    Interrupts::Initialize();
    Task::Initialize();

    #ifdef KERNEL_TESTS
      // spawn test runner task and start scheduling
      Task::Create(KernelTestRunner, 4096);
    #endif

    // enter scheduler; if no tests are queued we fall back to idle
    Task::Yield();
  }

  void Kernel::Panic(
    String message,
    String file,
    UInt32 line,
    String function
  ) {
    CString fileStr = file ? file : "unknown";
    CString funcStr = function ? function : "unknown";
    CString lineStr = nullptr;
    char lineBuffer[16] = {};

    if (
      line > 0 &&
      CStringHelper::ToCString(line, lineBuffer, sizeof(lineBuffer))
    ) {
      lineStr = lineBuffer;
    } else {
      lineStr = "unknown";
    }

    Logger::Write(LogLevel::Panic, ":( PANIC");

    char info[256] = {};
    Size out = 0;

    auto append = [&](CString src) -> bool {
      if (!src) {
        return true;
      } else {
        Size len = CStringHelper::Length(src);

        if (out + len >= sizeof(info)) {
          return false;
        } else {
          for (Size i = 0; i < len; ++i) {
            info[out++] = src[i];
          }

          return true;
        }
      }
    };

    CString trimmedFile = Helpers::DebugHelper::TrimSourceFile(fileStr);

    append(trimmedFile);
    append(":");
    append(lineStr);
    append(" (");
    append(funcStr);
    append(")");

    info[out] = '\0';

    Logger::Write(LogLevel::Panic, info);
    Logger::Write(LogLevel::Panic, message ? message : "unknown");

    CPU::HaltForever();
  }
}
