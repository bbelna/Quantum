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
#if defined(QUANTUM_ARCH_IA32)
#include <Arch/IA32/IO.hpp>
#endif

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

      case SystemCall::IO_In8: {
        if (!Task::HasIOAccess()) {
          context.eax = 1;

          break;
        }

        UInt16 port = static_cast<UInt16>(context.ebx);

        #if defined(QUANTUM_ARCH_IA32)
        context.eax = Arch::IA32::IO::In8(port);
        #else
        context.eax = 1;
        #endif

        break;
      }

      case SystemCall::IO_In16: {
        if (!Task::HasIOAccess()) {
          context.eax = 1;

          break;
        }

        UInt16 port = static_cast<UInt16>(context.ebx);

        #if defined(QUANTUM_ARCH_IA32)
        context.eax = Arch::IA32::IO::In16(port);
        #else
        context.eax = 1;
        #endif

        break;
      }

      case SystemCall::IO_In32: {
        if (!Task::HasIOAccess()) {
          context.eax = 1;

          break;
        }

        UInt16 port = static_cast<UInt16>(context.ebx);

        #if defined(QUANTUM_ARCH_IA32)
        context.eax = Arch::IA32::IO::In32(port);
        #else
        context.eax = 1;
        #endif

        break;
      }

      case SystemCall::IO_Out8: {
        if (!Task::HasIOAccess()) {
          context.eax = 1;

          break;
        }

        UInt16 port = static_cast<UInt16>(context.ebx);
        UInt8 value = static_cast<UInt8>(context.ecx);

        #if defined(QUANTUM_ARCH_IA32)
        Arch::IA32::IO::Out8(port, value);
        context.eax = 0;
        #else
        context.eax = 1;
        #endif

        break;
      }

      case SystemCall::IO_Out16: {
        if (!Task::HasIOAccess()) {
          context.eax = 1;

          break;
        }

        UInt16 port = static_cast<UInt16>(context.ebx);
        UInt16 value = static_cast<UInt16>(context.ecx);

        #if defined(QUANTUM_ARCH_IA32)
        Arch::IA32::IO::Out16(port, value);
        context.eax = 0;
        #else
        context.eax = 1;
        #endif

        break;
      }

      case SystemCall::IO_Out32: {
        if (!Task::HasIOAccess()) {
          context.eax = 1;

          break;
        }

        UInt16 port = static_cast<UInt16>(context.ebx);
        UInt32 value = context.ecx;

        #if defined(QUANTUM_ARCH_IA32)
        Arch::IA32::IO::Out32(port, value);
        context.eax = 0;
        #else
        context.eax = 1;
        #endif

        break;
      }

      case SystemCall::GrantIOAccess: {
        if (!Task::IsCurrentTaskCoordinator()) {
          context.eax = 1;

          break;
        }

        UInt32 targetId = context.ebx;
        bool ok = Task::GrantIOAccess(targetId);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::SpawnInitBundle: {
        if (!Task::IsCurrentTaskCoordinator()) {
          context.eax = 0;

          break;
        }

        CString name = reinterpret_cast<CString>(context.ebx);
        UInt32 taskId = Kernel::SpawnInitBundleTask(name);

        context.eax = taskId;

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
