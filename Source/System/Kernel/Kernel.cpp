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

#define MEMORY_TEST

namespace Quantum::Kernel {
  using CStringHelper = Helpers::CStringHelper;
  using LogLevel = Logger::Level;

  void Kernel::Initialize(UInt32 bootInfoPhysicalAddress) {
    Logger::WriteFormatted(
      LogLevel::Trace,
      "bootInfoPhysicalAddress=%p",
      bootInfoPhysicalAddress
    );

    Memory::Initialize(bootInfoPhysicalAddress);
    Memory::DumpState();
    Logger::Write(LogLevel::Info, "Initialized memory subsystem");

    Interrupts::Initialize();
    Logger::Write(LogLevel::Info, "Initialized interrupt subsystem");

    #ifdef MEMORY_TEST
      Memory::Test();
    #endif
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

    if (line > 0 && CStringHelper::ToCString(line, lineBuffer, sizeof(lineBuffer))) {
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
