//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Kernel.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// The core kernel implementation for Quantum.
//------------------------------------------------------------------------------

#include <Drivers/Console.hpp>
#include <Helpers/String.hpp>
#include <Interrupts.hpp>
#include <Kernel.hpp>
#include <Memory.hpp>

#if defined(QUANTUM_ARCH_IA32)
  #include <Arch/IA32/CPU.hpp>

  using ArchCPU = Quantum::Kernel::Arch::IA32::CPU;
#else
  #error "No architecture selected for kernel"
#endif

namespace Quantum::Kernel {
  using Helpers::String;

  using namespace Drivers;

  void Kernel::Initialize(uint32 bootInfoPhysicalAddress) {
    Console::Initialize();

    DumpVersionAndCopyright();

    Console::Write("bootInfoPhysicalAddress=");
    Console::WriteHex32(bootInfoPhysicalAddress);
    Console::WriteLine("");

    Memory::Initialize(bootInfoPhysicalAddress);
    Console::WriteLine("Initialized memory subsystem");

    Interrupts::Initialize();
    Console::WriteLine("Initialized interrupt subsystem");

    Panic(
      "End of Kernel::Initialize reached",
      __FILE__,
      __LINE__,
      __FUNCTION__
    );
  }

  void Kernel::DumpVersionAndCopyright() {
    // TODO: versioning, architecture info, build date, etc.
    Console::WriteLine("Quantum Kernel");
    Console::WriteLine("Copyright (c) 2025 Brandon Belna");
    Console::WriteLine("Released under the MIT License");
    Console::WriteLine("Provided \"AS IS\" without warranty\n");
  }

  void Kernel::Panic(
    const char* message,
    const char* file,
    int line,
    const char* function
  ) {
    const char* fileStr = file ? file : "unknown";
    const char* funcStr = function ? function : "unknown";

    char lineBuffer[16] = {};
    const char* lineStr = nullptr;

    if (line > 0 && String::ToString(line, lineBuffer, sizeof(lineBuffer))) {
      lineStr = lineBuffer;
    } else {
      lineStr = "unknown";
    }

    Console::WriteLine(":( PANIC");
    Console::WriteLine(message ? message : "unknown");

    char info[256] = {};
    usize out = 0;

    auto append = [&](const char* src) -> bool {
      if (!src) {
        return true;
      }

      usize len = String::Length(src);

      if (out + len >= sizeof(info)) {
        return false;
      }

      for (usize i = 0; i < len; ++i) {
        info[out++] = src[i];
      }

      return true;
    };

    // strip any prefix up to and including "/src/" or "\\src\\"
    const char* trimmedFile = fileStr;
    const char* slashSrc = nullptr;
    for (const char* p = fileStr; *p != '\0'; ++p) {
      if (
        (p[0] == '/' && p[1] == 's' && p[2] == 'r' && p[3] == 'c' && p[4] == '/')
        || (p[0] == '\\' && p[1] == 's' && p[2] == 'r' && p[3] == 'c' && p[4] == '\\')
      ) {
        slashSrc = p + 5; // skip past "/src/" or "\src\"
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

    ArchCPU::HaltForever();
  }
}
