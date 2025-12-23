/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Handlers/SystemCallHandler.cpp
 * System call handler.
 */

#include <ABI/Devices/BlockDevice.hpp>
#include <ABI/InitBundle.hpp>
#include <ABI/IPC.hpp>
#include <ABI/Prelude.hpp>
#include <ABI/SystemCall.hpp>

#include "Console.hpp"
#include "Devices/BlockDevice.hpp"
#include "Handlers/SystemCallHandler.hpp"
#include "InitBundle.hpp"
#include "Interrupts.hpp"
#include "IPC.hpp"
#include "Logger.hpp"
#include "Prelude.hpp"
#include "Task.hpp"

#if defined(QUANTUM_ARCH_IA32)
#include "Arch/IA32/IO.hpp"
#endif

// TODO: refactor into arch code
namespace Quantum::System::Kernel::Handlers {
  using BlockDevice = Devices::BlockDevice;
  using Console = Kernel::Console;
  using DMABuffer = ABI::Devices::BlockDevice::DMABuffer;
  using InitBundle = ABI::InitBundle;
  using IPC = ABI::IPC;
  using Logger = Kernel::Logger;
  using LogLevel = Logger::Level;
  using SystemCall = ABI::SystemCall;
  using Task = Kernel::Task;

  Interrupts::Context* SystemCallHandler::Handle(Interrupts::Context& context) {
    SystemCall id = static_cast<SystemCall>(context.eax);

    switch (id) {
      case SystemCall::Task_Exit: {
        Task::Exit();

        break;
      }

      case SystemCall::Task_Yield: {
        Task::Yield();

        break;
      }

      case SystemCall::Task_GrantIOAccess: {
        if (!Task::IsCurrentTaskCoordinator()) {
          context.eax = 1;

          break;
        }

        UInt32 targetId = context.ebx;
        bool ok = Task::GrantIOAccess(targetId);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::Console_Write: {
        CString string = reinterpret_cast<CString>(context.ebx);
        UInt32 length = context.ecx;

        Console::Write(string, length);

        break;
      }

      case SystemCall::Console_WriteLine: {
        CString string = reinterpret_cast<CString>(context.ebx);
        UInt32 length = context.ecx;

        Console::WriteLine(string, length);

        break;
      }

      case SystemCall::InitBundle_GetInfo: {
        InitBundle::Info* info
          = reinterpret_cast<InitBundle::Info*>(context.ebx);
        UInt32 base = 0;
        UInt32 size = 0;
        bool ok = Kernel::InitBundle::GetInfo(base, size);

        if (info) {
          info->base = base;
          info->size = size;
        }

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::InitBundle_SpawnTask: {
        if (!Task::IsCurrentTaskCoordinator()) {
          context.eax = 0;

          break;
        }

        CString name = reinterpret_cast<CString>(context.ebx);
        UInt32 taskId = Kernel::InitBundle::SpawnTask(name);

        context.eax = taskId;

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

      case SystemCall::Block_GetCount: {
        context.eax = BlockDevice::GetCount();

        break;
      }

      case SystemCall::Block_GetInfo: {
        UInt32 deviceId = context.ebx;
        BlockDevice::Info* info
          = reinterpret_cast<BlockDevice::Info*>(context.ecx);

        if (!info) {
          context.eax = 1;

          break;
        }

        bool ok = BlockDevice::GetInfo(deviceId, *info);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::Block_UpdateInfo: {
        UInt32 deviceId = context.ebx;
        BlockDevice::Info* info
          = reinterpret_cast<BlockDevice::Info*>(context.ecx);

        if (!info) {
          context.eax = 1;

          break;
        }

        bool ok = BlockDevice::UpdateInfo(deviceId, *info);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::Block_Read: {
        BlockDevice::Request* request
          = reinterpret_cast<BlockDevice::Request*>(context.ebx);

        if (!request) {
          context.eax = 1;

          break;
        }

        context.eax = BlockDevice::Read(*request) ? 0 : 1;

        break;
      }

      case SystemCall::Block_Write: {
        BlockDevice::Request* request
          = reinterpret_cast<BlockDevice::Request*>(context.ebx);

        if (!request) {
          context.eax = 1;

          break;
        }

        context.eax = BlockDevice::Write(*request) ? 0 : 1;

        break;
      }

      case SystemCall::Block_Bind: {
        UInt32 deviceId = context.ebx;
        UInt32 portId = context.ecx;

        bool ok = BlockDevice::Bind(deviceId, portId);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::Block_AllocateDMABuffer: {
        UInt32 sizeBytes = context.ebx;
        DMABuffer* buffer = reinterpret_cast<DMABuffer*>(context.ecx);

        if (!buffer) {
          context.eax = 1;

          break;
        }

        UInt32 physical = 0;
        UInt32 virtualAddress = 0;
        UInt32 outSize = 0;
        bool ok = BlockDevice::AllocateDMABuffer(
          sizeBytes,
          physical,
          virtualAddress,
          outSize
        );

        buffer->physical = physical;
        buffer->virtualAddress
          = reinterpret_cast<void*>(virtualAddress);
        buffer->size = outSize;

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
