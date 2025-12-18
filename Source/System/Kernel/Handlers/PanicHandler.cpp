/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Handlers/PanicHandler.cpp
 * Kernel panic handler.
 */

#include <CPU.hpp>
#include <Logger.hpp>
#include <Prelude.hpp>
#include <Handlers/PanicHandler.hpp>
#include <Helpers/CStringHelper.hpp>
#include <Helpers/DebugHelper.hpp>
#include <Types/Logging/LogLevel.hpp>

namespace Quantum::System::Kernel::Handlers {
  using Kernel::CPU;
  using Kernel::Logger;
  using Helpers::CStringHelper;
  using Types::Logging::LogLevel;

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

    CStringHelper::Concat(":( PANIC: ", message.Data(), panicMessage);

    Logger::Write(LogLevel::Panic, panicMessage);
    Logger::Write(LogLevel::Panic, info);
    Logger::Write(LogLevel::Panic, message ? message : "unknown");

    CPU::HaltForever();

    __builtin_unreachable();
  }
}
