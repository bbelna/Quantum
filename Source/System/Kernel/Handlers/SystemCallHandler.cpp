/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Handlers/SystemCallHandler.cpp
 * System call handler.
 */

#include <ABI/Prelude.hpp>
#include <ABI/InitBundle.hpp>
#include <ABI/IPC.hpp>
#include <ABI/SystemCall.hpp>
#include <Console.hpp>
#include <IPC.hpp>
#include <Interrupts.hpp>
#include <Handlers/SystemCallHandler.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Task.hpp>
#include <Prelude.hpp>

// TODO: refactor into arch code
namespace Quantum::System::Kernel::Handlers {
  using ABI::InitBundle;
  using ABI::IPC;
  using ABI::SystemCall;
  using Kernel::Console;
  using Kernel::Logger;
  using Kernel::Task;
  using LogLevel = Logger::Level;

  Interrupts::Context* SystemCallHandler::Handle(Interrupts::Context& context) {
    SystemCall id = static_cast<SystemCall>(context.eax);

    switch (id) {
      case SystemCall::Write: {
        CString buffer = reinterpret_cast<CString>(context.ebx);
        UInt32 length = context.ecx;

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
        InitBundle::Info* info
          = reinterpret_cast<InitBundle::Info*>(context.ebx);
        UInt32 base = 0;
        UInt32 size = 0;
        bool ok = Kernel::GetInitBundleInfo(base, size);

        if (info) {
          info->base = base;
          info->size = size;
        }

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::IPC_CreatePort: {
        UInt32 portId = Kernel::IPC::CreatePort();
        context.eax = portId == 0 ? 1u : portId;

        break;
      }

      case SystemCall::IPC_Send: {
        UInt32 portId = context.ebx;
        IPC::Message* msg = reinterpret_cast<IPC::Message*>(context.ecx);

        if (!msg || msg->length == 0 || msg->length > IPC::maxPayloadBytes) {
          context.eax = 1;
          break;
        }

        UInt32 sender = Kernel::Task::GetCurrentId();
        bool ok = Kernel::IPC::Send(portId, sender, msg->payload, msg->length);
        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::IPC_Receive: {
        UInt32 portId = context.ebx;
        IPC::Message* msg = reinterpret_cast<IPC::Message*>(context.ecx);

        if (!msg) {
          context.eax = 1;
          break;
        }

        UInt32 sender = 0;
        UInt32 length = 0;
        bool ok = Kernel::IPC::Receive(
          portId,
          sender,
          msg->payload,
          IPC::maxPayloadBytes,
          length
        );

        if (ok) {
          msg->senderId = sender;
          msg->length = length;
        }

        context.eax = ok ? 0 : 1;

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
