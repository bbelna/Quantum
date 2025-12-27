/**
 * @file System/Kernel/Arch/IA32/SystemCalls.cpp
 * @brief IA32 system call handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <ABI/Devices/BlockDevice.hpp>
#include <ABI/InitBundle.hpp>
#include <ABI/IPC.hpp>
#include <ABI/Prelude.hpp>
#include <ABI/SystemCall.hpp>
#include <Types.hpp>

#include "Arch/IA32/IDT.hpp"
#include "Arch/IA32/IO.hpp"
#include "Arch/IA32/Interrupts.hpp"
#include "Arch/IA32/SystemCalls.hpp"
#include "Console.hpp"
#include "Devices/BlockDevice.hpp"
#include "InitBundle.hpp"
#include "Interrupts.hpp"
#include "IPC.hpp"
#include "IRQ.hpp"
#include "Logger.hpp"
#include "Memory.hpp"
#include "Prelude.hpp"
#include "Task.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  using BlockDevice = Kernel::Devices::BlockDevice;
  using Console = Kernel::Console;
  using DMABuffer = ABI::Devices::BlockDevice::DMABuffer;
  using InitBundle = ABI::InitBundle;
  using IPC = ABI::IPC;
  using Logger = Kernel::Logger;
  using LogLevel = Kernel::Logger::Level;
  using Memory = Kernel::Memory;
  using SystemCall = ABI::SystemCall;
  using IRQ = Kernel::IRQ;

  extern "C" void SYSCALL80();

  Interrupts::Context* SystemCalls::OnSystemCall(Interrupts::Context& context) {
        SystemCall id = static_cast<SystemCall>(context.eax);

    switch (id) {
      case SystemCall::Task_Exit: {
        Kernel::Task::Exit();

        break;
      }

      case SystemCall::Task_Yield: {
        Kernel::Task::Yield();

        break;
      }

      case SystemCall::Task_GrantIOAccess: {
        if (!Kernel::Task::IsCurrentTaskCoordinator()) {
          context.eax = 1;

          break;
        }

        UInt32 targetId = context.ebx;
        bool ok = Kernel::Task::GrantIOAccess(targetId);

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
        if (!Kernel::Task::IsCurrentTaskCoordinator()) {
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

      case SystemCall::IPC_TryReceive: {
        UInt32 portId = context.ebx;
        IPC::Message* msg = reinterpret_cast<IPC::Message*>(context.ecx);

        if (!msg) {
          context.eax = 1;

          break;
        }

        UInt32 sender = 0;
        UInt32 length = 0;
        bool ok = Kernel::IPC::TryReceive(
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
        if (!Kernel::Task::CurrentTaskHasIOAccess()) {
          context.eax = 1;

          break;
        }

        UInt16 port = static_cast<UInt16>(context.ebx);

        context.eax = Arch::IA32::IO::In8(port);

        break;
      }

      case SystemCall::IO_In16: {
        if (!Kernel::Task::CurrentTaskHasIOAccess()) {
          context.eax = 1;

          break;
        }

        UInt16 port = static_cast<UInt16>(context.ebx);

        context.eax = Arch::IA32::IO::In16(port);

        break;
      }

      case SystemCall::IO_In32: {
        if (!Kernel::Task::CurrentTaskHasIOAccess()) {
          context.eax = 1;

          break;
        }

        UInt16 port = static_cast<UInt16>(context.ebx);

        context.eax = Arch::IA32::IO::In32(port);

        break;
      }

      case SystemCall::IO_Out8: {
        if (!Kernel::Task::CurrentTaskHasIOAccess()) {
          context.eax = 1;

          break;
        }

        UInt16 port = static_cast<UInt16>(context.ebx);
        UInt8 value = static_cast<UInt8>(context.ecx);

        IO::Out8(port, value);

        context.eax = 0;

        break;
      }

      case SystemCall::IO_Out16: {
        if (!Kernel::Task::CurrentTaskHasIOAccess()) {
          context.eax = 1;

          break;
        }

        UInt16 port = static_cast<UInt16>(context.ebx);
        UInt16 value = static_cast<UInt16>(context.ecx);

        IO::Out16(port, value);

        context.eax = 0;

        break;
      }

      case SystemCall::IO_Out32: {
        if (!Kernel::Task::CurrentTaskHasIOAccess()) {
          context.eax = 1;

          break;
        }

        UInt16 port = static_cast<UInt16>(context.ebx);
        UInt32 value = context.ecx;

        IO::Out32(port, value);

        context.eax = 0;

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

      case SystemCall::Block_Register: {
        BlockDevice::Info* info
          = reinterpret_cast<BlockDevice::Info*>(context.ebx);

        if (!info) {
          context.eax = 0;

          break;
        }

        context.eax = BlockDevice::RegisterUser(*info);

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

      case SystemCall::IRQ_Register: {
        if (!Kernel::Task::IsCurrentTaskCoordinator()) {
          context.eax = 1;

          break;
        }

        UInt32 irq = context.ebx;
        UInt32 portId = context.ecx;
        bool ok = IRQ::Register(irq, portId);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::IRQ_Unregister: {
        if (!Kernel::Task::IsCurrentTaskCoordinator()) {
          context.eax = 1;

          break;
        }

        UInt32 irq = context.ebx;
        bool ok = IRQ::Unregister(irq);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::IRQ_Enable: {
        if (!Kernel::Task::IsCurrentTaskCoordinator()) {
          context.eax = 1;

          break;
        }

        UInt32 irq = context.ebx;
        bool ok = IRQ::Enable(irq);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::IRQ_Disable: {
        if (!Kernel::Task::IsCurrentTaskCoordinator()) {
          context.eax = 1;

          break;
        }

        UInt32 irq = context.ebx;
        bool ok = IRQ::Disable(irq);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::Memory_ExpandHeap: {
        constexpr UInt32 pageSize = 4096;
        UInt32 sizeBytes = context.ebx;
        Kernel::Task::ControlBlock* tcb = Kernel::Task::GetCurrent();

        if (!tcb || tcb->userHeapLimit == 0) {
          context.eax = 0;

          break;
        }

        UInt32 heapEnd = tcb->userHeapEnd;

        if (sizeBytes == 0) {
          context.eax = heapEnd;

          break;
        }

        UInt64 newEnd64 = static_cast<UInt64>(heapEnd) + sizeBytes;

        if (newEnd64 > tcb->userHeapLimit) {
          context.eax = 0;

          break;
        }

        UInt32 newEnd = static_cast<UInt32>(newEnd64);
        UInt32 newMappedEnd
          = (newEnd + pageSize - 1) & ~(pageSize - 1);
        UInt32 mappedEnd = tcb->userHeapMappedEnd;

        if (mappedEnd == 0) {
          mappedEnd = tcb->userHeapBase;
        }

        bool ok = true;
        UInt32 mappedProgress = mappedEnd;

        if (newMappedEnd > mappedEnd) {
          UInt32 addressSpace = Kernel::Task::GetCurrentAddressSpace();

          for (
            UInt32 vaddr = mappedEnd;
            vaddr < newMappedEnd;
            vaddr += pageSize
          ) {
            void* phys = Memory::AllocatePage(true);

            if (!phys) {
              ok = false;

              break;
            }

            Memory::MapPageInAddressSpace(
              addressSpace,
              vaddr,
              reinterpret_cast<UInt32>(phys),
              true,
              true,
              false
            );

            mappedProgress = vaddr + pageSize;
          }
        }

        if (!ok) {
          tcb->userHeapMappedEnd = mappedProgress;
          context.eax = 0;

          break;
        }

        tcb->userHeapEnd = newEnd;
        tcb->userHeapMappedEnd = newMappedEnd;
        context.eax = heapEnd;

        break;
      }

      default: {
        Logger::WriteFormatted(LogLevel::Warning, "Unknown SystemCall %p", id);

        break;
      }
    }

    return &context;
  }

  void SystemCalls::Initialize() {
    IDT::SetGate(vector, SYSCALL80, 0xEE);
    Interrupts::RegisterHandler(vector, OnSystemCall);
  }
}
