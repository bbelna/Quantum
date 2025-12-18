/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Handlers/SystemCallHandler.cpp
 * System call handler.
 */

#include <Console.hpp>
#include <IPC.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Task.hpp>
#include <Prelude.hpp>
#include <ABI/Types/InitBundleInfo.hpp>
#include <ABI/Types/IPC.hpp>
#include <ABI/Types/SystemCall.hpp>
#include <Handlers/SystemCallHandler.hpp>
#include <Types/Logging/LogLevel.hpp>

namespace Quantum::System::Kernel::Handlers {
  using ::Quantum::ABI::Types::InitBundleInfo;
  using ::Quantum::ABI::Types::IPC::Message;
  using ::Quantum::ABI::Types::SystemCall;
  using Kernel::Console;
  using Kernel::IPC::MaxPayloadBytes;
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

      case SystemCall::IPC_CreatePort: {
        UInt32 portId = Kernel::IPC::CreatePort();
        context.EAX = portId == 0 ? 1u : portId;

        break;
      }

      case SystemCall::IPC_Send: {
        UInt32 portId = context.EBX;
        Message* msg = reinterpret_cast<Message*>(context.ECX);

        if (!msg || msg->Length == 0 || msg->Length > MaxPayloadBytes) {
          context.EAX = 1;
          break;
        }

        UInt32 sender = Kernel::Task::GetCurrentId();
        bool ok = Kernel::IPC::Send(portId, sender, msg->Payload, msg->Length);
        context.EAX = ok ? 0 : 1;

        break;
      }

      case SystemCall::IPC_Receive: {
        UInt32 portId = context.EBX;
        Message* msg = reinterpret_cast<Message*>(context.ECX);

        if (!msg) {
          context.EAX = 1;
          break;
        }

        UInt32 sender = 0;
        UInt32 length = 0;
        bool ok = Kernel::IPC::Receive(
          portId,
          sender,
          msg->Payload,
          MaxPayloadBytes,
          length
        );

        if (ok) {
          msg->SenderId = sender;
          msg->Length = length;
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
