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
#include <Helpers/CStringHelper.hpp>
#include <Helpers/DebugHelper.hpp>
#include <Arch/IA32/Memory.hpp>
#include <Arch/IA32/UserMode.hpp>
#include <Types/Primitives.hpp>
#include <Types/Logging/LogLevel.hpp>

namespace Quantum::System::Kernel {
  using CStringHelper = Helpers::CStringHelper;
  using LogLevel = Types::Logging::LogLevel;

  namespace {
    constexpr UInt32 _userProgramBase = 0x00400000;
    constexpr UInt32 _userStackTop = 0x00800000;
    constexpr UInt32 _userStackSize = 16 * 4096;

    extern "C" UInt8 _binary_Build_Coordinator_Coordinator_bin_start[];
    extern "C" UInt8 _binary_Build_Coordinator_Coordinator_bin_end[];

    void LaunchCoordinatorUser() {
      UInt32 size = static_cast<UInt32>(
        _binary_Build_Coordinator_Coordinator_bin_end
        - _binary_Build_Coordinator_Coordinator_bin_start
      );

      UInt32 pages = (size + 4095) / 4096;

      for (UInt32 i = 0; i < pages; ++i) {
        void* phys = Memory::AllocatePage(true);
        UInt32 vaddr = _userProgramBase + i * 4096;

        Arch::IA32::Memory::MapPage(
          vaddr,
          reinterpret_cast<UInt32>(phys),
          true,
          true,
          false
        );
      }

      UInt8* dest = reinterpret_cast<UInt8*>(_userProgramBase);

      for (UInt32 i = 0; i < size; ++i) {
        dest[i] = _binary_Build_Coordinator_Coordinator_bin_start[i];
      }

      Arch::IA32::UserMode::MapUserStack(_userStackTop, _userStackSize);
      UInt32 entryOffset = *reinterpret_cast<UInt32*>(_userProgramBase);
      Arch::IA32::UserMode::Enter(
        _userProgramBase + entryOffset,
        _userStackTop
      );
    }

    /**
     * Kernel test runner task entry point.
     */
    void KernelTestRunner() {
      Testing::RegisterBuiltins();
      Testing::RunAll();
      Logger::Write(LogLevel::Info, "Kernel tests task finished");
      Task::Exit();
    }
  }

  void Initialize(UInt32 bootInfoPhysicalAddress) {
    Memory::Initialize(bootInfoPhysicalAddress);
    Interrupts::Initialize();
    Task::Initialize();
    Task::EnablePreemption();

    Task::Create(LaunchCoordinatorUser, 4096);

    #ifdef KERNEL_TESTS
      // spawn test runner task and start scheduling
      Task::Create(KernelTestRunner, 4096);
    #endif

    // enter scheduler; if no tests are queued we fall back to idle
    Task::Yield();
  }

  void Panic(
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

    char panicMessage[256] = {};

    CStringHelper::Concat(":( PANIC: ", message.Data(), panicMessage);
    Logger::Write(LogLevel::Panic, panicMessage);

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
