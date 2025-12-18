/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/SystemCall.cpp
 * System call dispatcher implementation.
 */

#include <Console.hpp>
#include <Logger.hpp>
#include <SystemCall.hpp>
#include <Task.hpp>
#include <Types/Logging/LogLevel.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel {
  using LogLevel = Types::Logging::LogLevel;

  InterruptContext* SystemCall::Handle(InterruptContext& context) {
    SystemCallId id = static_cast<SystemCallId>(context.EAX);

    switch (id) {
      case SystemCallId::Write: {
        const char* buffer = reinterpret_cast<const char*>(context.EBX);
        UInt32 length = context.ECX;

        Console::Write(buffer, length);

        break;
      }

      case SystemCallId::Exit: {
        Task::Exit();

        break;
      }

      case SystemCallId::Yield: {
        Task::Yield();

        break;
      }

      default: {
        Logger::Write(LogLevel::Warning, "Unknown SystemCall id");

        break;
      }
    }

    return &context;
  }
}
