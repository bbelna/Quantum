/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Handlers/PanicHandler.cpp
 * Kernel panic handler.
 */

#include "CPU.hpp"
#include "Handlers/PanicHandler.hpp"
#include "Helpers/CStringHelper.hpp"
#include "Helpers/DebugHelper.hpp"
#include "Logger.hpp"
#include "Prelude.hpp"

namespace Quantum::System::Kernel::Handlers {
  using CPU = Kernel::CPU;
  using Logger = Kernel::Logger;
  using CStringHelper = Helpers::CStringHelper;
  using LogLevel = Logger::Level;

  void PanicHandler::Handle(
    String message,
    String file,
    UInt32 line,
    String function
  ) {
    CString info = Helpers::DebugHelper::GetPanicInfo(
      file,
      line,
      function
    );
    char panicMessage[256] = {};

    CStringHelper::Concat(
      "  ",
      message ? message : "unknown",
      panicMessage,
      sizeof(panicMessage)
    );

    Logger::Write(LogLevel::Panic, ":( PANIC");
    Logger::Write(LogLevel::Panic, panicMessage);
    Logger::Write(LogLevel::Panic, info);

    CPU::HaltForever();

    __builtin_unreachable();
  }
}
