/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Handlers/SystemCallHandler.cpp
 * System call handler.
 */

#include <Console.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Task.hpp>
#include <Prelude.hpp>
#include <ABI/Types/InitBundleInfo.hpp>
#include <ABI/Types/SystemCall.hpp>
#include <Handlers/SystemCallHandler.hpp>
#include <Types/Logging/LogLevel.hpp>

namespace Quantum::System::Kernel::Handlers {
  using ::Quantum::ABI::Types::InitBundleInfo;
  using ::Quantum::ABI::Types::SystemCall;
  using Kernel::Console;
  using Kernel::Logger;
  using Kernel::Task;
  using Kernel::Types::Logging::LogLevel;

  InterruptContext* SystemCallHandler::Handle(InterruptContext& context) {
    SystemCall id = static_cast<SystemCall>(context.EAX);

    switch (id) {
      case SystemCall::Write: {
        CString buffer = reinterpret_cast<CString>(context.EBX);
        UInt32 length = context.ECX;

        Console::Write(buffer, length);

        break;
      }

      case SystemCall::Exit: {
        Task::Exit();

        break;
      }

      case SystemCall::Yield: {
        Task::Yield();

        break;
      }

      case SystemCall::GetInitBundleInfo: {
        InitBundleInfo* info
          = reinterpret_cast<InitBundleInfo*>(context.EBX);
        UInt32 base = 0;
        UInt32 size = 0;
        bool ok = Kernel::GetInitBundleInfo(base, size);

        if (info) {
          info->Base = base;
          info->Size = size;
        }

        context.EAX = ok ? 0 : 1;

        break;
      }

      default: {
        Logger::WriteFormatted(LogLevel::Warning, "Unknown SystemCall %p", id);

        break;
      }
    }

    return &context;
  }
}
