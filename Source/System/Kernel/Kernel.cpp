//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Kernel.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// The core kernel implementation for Quantum.
//------------------------------------------------------------------------------

#include <CPU.hpp>
#include <Helpers/CStringHelper.hpp>
#include <Interrupts.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Memory.hpp>
#include <Task.hpp>

#define MEMORY_TEST
#define TASK_TEST

namespace Quantum::Kernel {
  using CStringHelper = Helpers::CStringHelper;
  using LogLevel = Logger::Level;

  namespace {
    /**
     * Test task 1 - prints messages and yields.
     */
    void TestTask1() {
      for (int i = 0; i < 5; i++) {
        Logger::WriteFormatted(LogLevel::Info, "Task 1: iteration %d", i);
        Task::Yield();
      }

      Logger::Write(LogLevel::Info, "Task 1: completed");
    }

    /**
     * Test task 2 - prints messages and yields.
     */
    void TestTask2() {
      for (int i = 0; i < 5; i++) {
        Logger::WriteFormatted(LogLevel::Info, "Task 2: iteration %d", i);
        Task::Yield();
      }

      Logger::Write(LogLevel::Info, "Task 2: completed");
    }

    /**
     * Test task 3 - prints messages and yields.
     */
    void TestTask3() {
      for (int i = 0; i < 3; i++) {
        Logger::WriteFormatted(LogLevel::Info, "Task 3: iteration %d", i);
        Task::Yield();
      }

      Logger::Write(LogLevel::Info, "Task 3: completed");
    }
  }

  void Kernel::Initialize(UInt32 bootInfoPhysicalAddress) {
    Memory::Initialize(bootInfoPhysicalAddress);
    Interrupts::Initialize();
    Task::Initialize();
    Task::Yield();

    // #ifdef TASK_TEST
    //   // create test tasks to verify multitasking works
    //   Logger::Write(LogLevel::Info, "Creating test tasks");
    //   Task::Create(TestTask1, 4096);
    //   Task::Create(TestTask2, 4096);
    //   Task::Create(TestTask3, 4096);

    //   // yield to start task switching
    //   Logger::Write(LogLevel::Info, "Starting multitasking");
      

    //   Logger::Write(LogLevel::Info, "All test tasks completed");
    // #endif
  }

  void Kernel::Panic(
    String message,
    String file,
    UInt32 line,
    String function
  ) {
    const char* fileStr = file ? file : "unknown";
    const char* funcStr = function ? function : "unknown";
    char lineBuffer[16] = {};
    const char* lineStr = nullptr;

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

    auto append = [&](const char* src) -> bool {
      if (!src) {
        return true;
      }

      Size len = CStringHelper::Length(src);

      if (out + len >= sizeof(info)) {
        return false;
      }

      for (Size i = 0; i < len; ++i) {
        info[out++] = src[i];
      }

      return true;
    };

    // strip any prefix up to and including "/Source/" or "\\Source\\"
    const char* trimmedFile = fileStr;
    const char* slashSrc = nullptr;

    for (const char* p = fileStr; *p != '\0'; ++p) {
      if (
        (p[0] == '/' || p[0] == '\\') &&
        p[1] == 'S' &&
        p[2] == 'o' &&
        p[3] == 'u' &&
        p[4] == 'r' &&
        p[5] == 'c' &&
        p[6] == 'e' &&
        (p[7] == '/' || p[7] == '\\')
      ) {
        slashSrc = p + 8;
      }
    }

    if (slashSrc) {
      trimmedFile = slashSrc;
    }

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
