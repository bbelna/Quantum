//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Kernel.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// The core kernel implementation for Quantum.
//------------------------------------------------------------------------------

#include <Drivers/Console.hpp>
#include <Helpers/CStringHelper.hpp>
#include <Interrupts.hpp>
#include <Kernel.hpp>
#include <Memory.hpp>

#if defined(QUANTUM_ARCH_IA32)
  #include <Arch/IA32/CPU.hpp>

  using ArchCPU = Quantum::Kernel::Arch::IA32::CPU;
#else
  #error "No architecture selected for kernel"
#endif

#define MEMORY_TEST

// TODO: kernel logging/tracing shouldn't use Console directly; abstract it
namespace Quantum::Kernel {
  using CStringHelper = Helpers::CStringHelper;

  using namespace Drivers;

  void Kernel::Initialize(UInt32 bootInfoPhysicalAddress) {
    Console::Initialize();

    TraceVersionAndCopyright();

    Console::Write("bootInfoPhysicalAddress=");
    Console::WriteHex32(bootInfoPhysicalAddress);
    Console::WriteLine("");

    Memory::Initialize(bootInfoPhysicalAddress);
    Console::WriteLine("Initialized memory subsystem");

    Memory::DumpState();

    Interrupts::Initialize();
    Console::WriteLine("Initialized interrupt subsystem");

    #ifdef MEMORY_TEST
      Memory::Test();
    #endif

    PANIC("End of kernel initialization");
  }

  void Kernel::TraceVersionAndCopyright() {
    // TODO: versioning, architecture info, build date, etc.
    Console::WriteLine("Quantum Kernel");
    Console::WriteLine("Copyright (c) 2025 Brandon Belna");
    Console::WriteLine("Released under the MIT License");
    Console::WriteLine("Provided \"AS IS\" without warranty\n");
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

    Console::WriteLine(":( PANIC");

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

    Console::WriteLine(info);
    Console::WriteLine(message ? message : "unknown");

    ArchCPU::HaltForever();
  }
}
