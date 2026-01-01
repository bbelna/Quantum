/**
 * @file System/Kernel/Arch/IA32/SystemCalls.cpp
 * @brief IA32 system call handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Devices/BlockDevices.hpp>
#include <ABI/Devices/InputDevices.hpp>
#include <ABI/Handle.hpp>
#include <ABI/InitBundle.hpp>
#include <ABI/IPC.hpp>
#include <ABI/IRQ.hpp>
#include <ABI/Prelude.hpp>
#include <ABI/SystemCall.hpp>
#include <Types.hpp>

#include "Arch/AddressSpace.hpp"
#include "Arch/IA32/IDT.hpp"
#include "Arch/IA32/IO.hpp"
#include "Arch/IA32/Interrupts.hpp"
#include "Arch/IA32/SystemCalls.hpp"
#include "Arch/IA32/PhysicalAllocator.hpp"
#include "Console.hpp"
#include "Devices/BlockDevices.hpp"
#include "Devices/InputDevices.hpp"
#include "InitBundle.hpp"
#include "Interrupts.hpp"
#include "IPC.hpp"
#include "Handles.hpp"
#include "IRQ.hpp"
#include "Logger.hpp"
#include "Prelude.hpp"
#include "Task.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  using ABI::InitBundle;
  using ABI::IPC;
  using ABI::SystemCall;
  using Kernel::Console;
  using Kernel::Devices::BlockDevices;
  using Kernel::Devices::InputDevices;
  using Kernel::HandleTable;
  using Kernel::Objects::IPCPortObject;
  using Kernel::Objects::KernelObject;
  using Kernel::Objects::KernelObjectType;
  using Kernel::IRQ;
  using Kernel::Logger;
  using Kernel::Objects::IRQLineObject;
  using Kernel::Objects::Devices::BlockDeviceObject;
  using Kernel::Objects::Devices::InputDeviceObject;

  using DMABuffer = ABI::Devices::BlockDevices::DMABuffer;
  using LogLevel = Kernel::Logger::Level;

  extern "C" void SYSCALL80();

  static bool ResolveIPCHandle(
    UInt32 portOrHandle,
    UInt32 rights,
    UInt32& outPortId
  ) {
    if (!HandleTable::IsHandle(portOrHandle)) {
      outPortId = portOrHandle;

      return true;
    }

    Kernel::Task::ControlBlock* tcb = Kernel::Task::GetCurrent();

    if (!tcb || !tcb->handleTable) {
      return false;
    }

    KernelObject* object = nullptr;

    if (!tcb->handleTable->Resolve(
      portOrHandle,
      KernelObjectType::IPCPort,
      rights,
      object
    )) {
      return false;
    }

    if (!object || object->type != KernelObjectType::IPCPort) {
      return false;
    }

    auto* portObject = reinterpret_cast<IPCPortObject*>(object);

    outPortId = portObject->portId;

    return true;
  }

  static bool ResolveBlockDeviceHandle(
    UInt32 deviceOrHandle,
    UInt32 rights,
    UInt32& outDeviceId
  ) {
    if (!HandleTable::IsHandle(deviceOrHandle)) {
      outDeviceId = deviceOrHandle;

      return true;
    }

    Kernel::Task::ControlBlock* tcb = Kernel::Task::GetCurrent();

    if (!tcb || !tcb->handleTable) {
      return false;
    }

    KernelObject* object = nullptr;

    if (!tcb->handleTable->Resolve(
      deviceOrHandle,
      KernelObjectType::BlockDevice,
      rights,
      object
    )) {
      return false;
    }

    auto* deviceObject = reinterpret_cast<BlockDeviceObject*>(object);

    outDeviceId = deviceObject->deviceId;

    return true;
  }

  static bool ResolveInputDeviceHandle(
    UInt32 deviceOrHandle,
    UInt32 rights,
    UInt32& outDeviceId
  ) {
    if (!HandleTable::IsHandle(deviceOrHandle)) {
      outDeviceId = deviceOrHandle;

      return true;
    }

    Kernel::Task::ControlBlock* tcb = Kernel::Task::GetCurrent();

    if (!tcb || !tcb->handleTable) {
      return false;
    }

    KernelObject* object = nullptr;

    if (!tcb->handleTable->Resolve(
      deviceOrHandle,
      KernelObjectType::InputDevice,
      rights,
      object
    )) {
      return false;
    }

    auto* deviceObject = reinterpret_cast<InputDeviceObject*>(object);

    outDeviceId = deviceObject->deviceId;

    return true;
  }

  static bool ResolveIRQHandle(
    UInt32 irqOrHandle,
    UInt32 rights,
    UInt32& outIRQ
  ) {
    if (!HandleTable::IsHandle(irqOrHandle)) {
      outIRQ = irqOrHandle;

      return true;
    }

    Kernel::Task::ControlBlock* tcb = Kernel::Task::GetCurrent();

    if (!tcb || !tcb->handleTable) {
      return false;
    }

    KernelObject* object = nullptr;

    if (!tcb->handleTable->Resolve(
      irqOrHandle,
      KernelObjectType::IRQLine,
      rights,
      object
    )) {
      return false;
    }

    auto* irqObject = reinterpret_cast<IRQLineObject*>(object);

    outIRQ = irqObject->irqLine;

    return true;
  }

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

        context.eax = portId;

        break;
      }

      case SystemCall::IPC_Send: {
        UInt32 portId = 0;
        UInt32 portOrHandle = context.ebx;
        IPC::Message* msg = reinterpret_cast<IPC::Message*>(context.ecx);

        if (!msg || msg->length == 0 || msg->length > IPC::maxPayloadBytes) {
          context.eax = 1;

          break;
        }

        if (!ResolveIPCHandle(portOrHandle, IPC::RightSend, portId)) {
          context.eax = 1;

          break;
        }

        UInt32 sender = Kernel::Task::GetCurrentId();
        bool ok = Kernel::IPC::Send(portId, sender, msg->payload, msg->length);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::IPC_Receive: {
        UInt32 portId = 0;
        UInt32 portOrHandle = context.ebx;
        IPC::Message* msg = reinterpret_cast<IPC::Message*>(context.ecx);

        if (!msg) {
          context.eax = 1;

          break;
        }

        if (!ResolveIPCHandle(portOrHandle, IPC::RightReceive, portId)) {
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
        UInt32 portId = 0;
        UInt32 portOrHandle = context.ebx;
        IPC::Message* msg = reinterpret_cast<IPC::Message*>(context.ecx);

        if (!msg) {
          context.eax = 1;

          break;
        }

        if (!ResolveIPCHandle(portOrHandle, IPC::RightReceive, portId)) {
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

      case SystemCall::IPC_DestroyPort: {
        UInt32 portId = 0;
        UInt32 portOrHandle = context.ebx;
        bool ok = false;

        if (!ResolveIPCHandle(portOrHandle, IPC::RightManage, portId)) {
          context.eax = 1;

          break;
        }

        UInt32 ownerId = 0;

        if (Kernel::IPC::GetPortOwner(portId, ownerId)) {
          if (ownerId == Kernel::Task::GetCurrentId()) {
            ok = Kernel::IPC::DestroyPort(portId);
          }
        }

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::IPC_OpenPort: {
        UInt32 portId = context.ebx;
        UInt32 rights = context.ecx;

        if (portId == 0) {
          context.eax = 0;

          break;
        }

        UInt32 ownerId = 0;

        if (!Kernel::IPC::GetPortOwner(portId, ownerId)) {
          context.eax = 0;

          break;
        }

        Kernel::Task::ControlBlock* tcb = Kernel::Task::GetCurrent();

        if (!tcb || !tcb->handleTable) {
          context.eax = 0;

          break;
        }

        UInt32 allowed = IPC::RightSend;

        if (ownerId == Kernel::Task::GetCurrentId()) {
          allowed |= IPC::RightReceive | IPC::RightManage;
        }

        if ((rights & ~allowed) != 0) {
          context.eax = 0;

          break;
        }

        rights &= allowed;

        if (rights == 0) {
          context.eax = 0;

          break;
        }

        IPCPortObject* portObject = Kernel::IPC::GetPortObject(portId);

        if (!portObject) {
          context.eax = 0;

          break;
        }

        HandleTable::Handle handle = tcb->handleTable->Create(
          KernelObjectType::IPCPort,
          portObject,
          rights
        );

        context.eax = handle;

        break;
      }

      case SystemCall::IPC_CloseHandle: {
        UInt32 handle = context.ebx;
        Kernel::Task::ControlBlock* tcb = Kernel::Task::GetCurrent();
        bool ok = false;

        if (tcb && tcb->handleTable && HandleTable::IsHandle(handle)) {
          ok = tcb->handleTable->Close(handle);
        }

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::IPC_SendHandle: {
        UInt32 portId = 0;
        UInt32 portOrHandle = context.ebx;
        UInt32 handle = context.ecx;
        UInt32 rights = context.edx;

        if (!ResolveIPCHandle(portOrHandle, IPC::RightSend, portId)) {
          context.eax = 1;

          break;
        }

        Kernel::Task::ControlBlock* tcb = Kernel::Task::GetCurrent();

        if (!tcb || !tcb->handleTable || !HandleTable::IsHandle(handle)) {
          context.eax = 1;

          break;
        }

        KernelObjectType type = KernelObjectType::None;
        UInt32 entryRights = 0;

        if (!tcb->handleTable->Query(handle, type, entryRights)) {
          context.eax = 1;

          break;
        }

        if (rights == 0) {
          rights = entryRights;
        } else if ((entryRights & rights) != rights) {
          context.eax = 1;

          break;
        }

        KernelObject* object = nullptr;

        if (!tcb->handleTable->Resolve(handle, type, rights, object)) {
          context.eax = 1;

          break;
        }

        UInt32 sender = Kernel::Task::GetCurrentId();
        bool ok = Kernel::IPC::SendHandle(portId, sender, object, rights);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::Handle_Close: {
        UInt32 handle = context.ebx;
        Kernel::Task::ControlBlock* tcb = Kernel::Task::GetCurrent();
        bool ok = false;

        if (tcb && tcb->handleTable && HandleTable::IsHandle(handle)) {
          ok = tcb->handleTable->Close(handle);
        }

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::Handle_Dup: {
        UInt32 handle = context.ebx;
        UInt32 rights = context.ecx;
        Kernel::Task::ControlBlock* tcb = Kernel::Task::GetCurrent();

        if (!tcb || !tcb->handleTable || !HandleTable::IsHandle(handle)) {
          context.eax = 0;

          break;
        }

        context.eax = tcb->handleTable->Duplicate(handle, rights);

        break;
      }

      case SystemCall::Handle_Query: {
        UInt32 handle = context.ebx;
        ABI::Handle::Info* info
          = reinterpret_cast<ABI::Handle::Info*>(context.ecx);
        Kernel::Task::ControlBlock* tcb = Kernel::Task::GetCurrent();

        if (!tcb || !tcb->handleTable || !HandleTable::IsHandle(handle) || !info) {
          context.eax = 1;

          break;
        }

        KernelObjectType type = KernelObjectType::None;
        UInt32 rights = 0;

        if (!tcb->handleTable->Query(handle, type, rights)) {
          context.eax = 1;

          break;
        }

        info->type = static_cast<UInt32>(type);
        info->rights = rights;
        context.eax = 0;

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
        context.eax = BlockDevices::GetCount();

        break;
      }

      case SystemCall::Block_GetInfo: {
        UInt32 deviceId = 0;
        UInt32 deviceOrHandle = context.ebx;
        BlockDevices::Info* info
          = reinterpret_cast<BlockDevices::Info*>(context.ecx);

        if (!info) {
          context.eax = 1;

          break;
        }

        if (!ResolveBlockDeviceHandle(
          deviceOrHandle,
          ABI::Devices::BlockDevices::RightControl,
          deviceId
        )) {
          context.eax = 1;

          break;
        }

        bool ok = BlockDevices::GetInfo(deviceId, *info);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::Block_Register: {
        BlockDevices::Info* info
          = reinterpret_cast<BlockDevices::Info*>(context.ebx);

        if (!info) {
          context.eax = 0;

          break;
        }

        context.eax = BlockDevices::RegisterUser(*info);

        break;
      }

      case SystemCall::Block_UpdateInfo: {
        UInt32 deviceId = 0;
        UInt32 deviceOrHandle = context.ebx;
        BlockDevices::Info* info
          = reinterpret_cast<BlockDevices::Info*>(context.ecx);

        if (!info) {
          context.eax = 1;

          break;
        }

        if (!ResolveBlockDeviceHandle(
          deviceOrHandle,
          ABI::Devices::BlockDevices::RightControl,
          deviceId
        )) {
          context.eax = 1;

          break;
        }

        bool ok = BlockDevices::UpdateInfo(deviceId, *info);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::Block_Read: {
        BlockDevices::Request* request
          = reinterpret_cast<BlockDevices::Request*>(context.ebx);

        if (!request) {
          context.eax = 1;

          break;
        }

        UInt32 deviceId = 0;

        if (!ResolveBlockDeviceHandle(
          request->deviceId,
          ABI::Devices::BlockDevices::RightRead,
          deviceId
        )) {
          context.eax = 1;

          break;
        }

        BlockDevices::Request resolved = *request;

        resolved.deviceId = deviceId;

        context.eax = BlockDevices::Read(resolved) ? 0 : 1;

        break;
      }

      case SystemCall::Block_Write: {
        BlockDevices::Request* request
          = reinterpret_cast<BlockDevices::Request*>(context.ebx);

        if (!request) {
          context.eax = 1;

          break;
        }

        UInt32 deviceId = 0;

        if (!ResolveBlockDeviceHandle(
          request->deviceId,
          ABI::Devices::BlockDevices::RightWrite,
          deviceId
        )) {
          context.eax = 1;

          break;
        }

        BlockDevices::Request resolved = *request;

        resolved.deviceId = deviceId;

        context.eax = BlockDevices::Write(resolved) ? 0 : 1;

        break;
      }

      case SystemCall::Block_Bind: {
        UInt32 deviceId = 0;
        UInt32 deviceOrHandle = context.ebx;
        UInt32 portId = context.ecx;

        if (!ResolveBlockDeviceHandle(
          deviceOrHandle,
          ABI::Devices::BlockDevices::RightBind,
          deviceId
        )) {
          context.eax = 1;

          break;
        }

        bool ok = BlockDevices::Bind(deviceId, portId);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::Block_Open: {
        if (!Kernel::Task::IsCurrentTaskCoordinator()) {
          context.eax = 0;

          break;
        }

        UInt32 deviceId = context.ebx;
        UInt32 rights = context.ecx;

        if (rights == 0) {
          context.eax = 0;

          break;
        }

        Kernel::Task::ControlBlock* tcb = Kernel::Task::GetCurrent();

        if (!tcb || !tcb->handleTable) {
          context.eax = 0;

          break;
        }

        KernelObject* object = BlockDevices::GetObject(deviceId);

        if (!object) {
          context.eax = 0;

          break;
        }

        UInt32 allowed = ABI::Devices::BlockDevices::RightRead
          | ABI::Devices::BlockDevices::RightWrite
          | ABI::Devices::BlockDevices::RightControl
          | ABI::Devices::BlockDevices::RightBind;

        rights &= allowed;

        if (rights == 0) {
          context.eax = 0;

          break;
        }

        HandleTable::Handle handle = tcb->handleTable->Create(
          KernelObjectType::BlockDevice,
          object,
          rights
        );

        context.eax = handle;

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
        bool ok = BlockDevices::AllocateDMABuffer(
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

      case SystemCall::Input_GetCount: {
        context.eax = InputDevices::GetCount();

        break;
      }

      case SystemCall::Input_GetInfo: {
        UInt32 deviceId = 0;
        UInt32 deviceOrHandle = context.ebx;
        InputDevices::Info* info
          = reinterpret_cast<InputDevices::Info*>(context.ecx);

        if (!info) {
          context.eax = 1;

          break;
        }

        if (!ResolveInputDeviceHandle(
          deviceOrHandle,
          ABI::Devices::InputDevices::RightControl,
          deviceId
        )) {
          context.eax = 1;

          break;
        }

        bool ok = InputDevices::GetInfo(deviceId, *info);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::Input_Register: {
        InputDevices::Info* info
          = reinterpret_cast<InputDevices::Info*>(context.ebx);

        if (!info) {
          context.eax = 0;

          break;
        }

        context.eax = InputDevices::RegisterUser(*info);

        break;
      }

      case SystemCall::Input_UpdateInfo: {
        UInt32 deviceId = 0;
        UInt32 deviceOrHandle = context.ebx;
        InputDevices::Info* info
          = reinterpret_cast<InputDevices::Info*>(context.ecx);

        if (!info) {
          context.eax = 1;

          break;
        }

        if (!ResolveInputDeviceHandle(
          deviceOrHandle,
          ABI::Devices::InputDevices::RightControl,
          deviceId
        )) {
          context.eax = 1;

          break;
        }

        bool ok = InputDevices::UpdateInfo(deviceId, *info);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::Input_ReadEvent: {
        UInt32 deviceId = 0;
        UInt32 deviceOrHandle = context.ebx;
        InputDevices::Event* event
          = reinterpret_cast<InputDevices::Event*>(context.ecx);

        if (!event) {
          context.eax = 1;

          break;
        }

        if (!ResolveInputDeviceHandle(
          deviceOrHandle,
          ABI::Devices::InputDevices::RightRead,
          deviceId
        )) {
          context.eax = 1;

          break;
        }

        bool ok = InputDevices::ReadEvent(deviceId, *event);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::Input_PushEvent: {
        UInt32 deviceId = 0;
        UInt32 deviceOrHandle = context.ebx;
        InputDevices::Event* event
          = reinterpret_cast<InputDevices::Event*>(context.ecx);

        if (!event) {
          context.eax = 1;

          break;
        }

        if (!ResolveInputDeviceHandle(
          deviceOrHandle,
          ABI::Devices::InputDevices::RightRegister,
          deviceId
        )) {
          context.eax = 1;

          break;
        }

        bool ok = InputDevices::PushEvent(deviceId, *event);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::Input_Open: {
        if (!Kernel::Task::IsCurrentTaskCoordinator()) {
          context.eax = 0;

          break;
        }

        UInt32 deviceId = context.ebx;
        UInt32 rights = context.ecx;

        if (rights == 0) {
          context.eax = 0;

          break;
        }

        Kernel::Task::ControlBlock* tcb = Kernel::Task::GetCurrent();

        if (!tcb || !tcb->handleTable) {
          context.eax = 0;

          break;
        }

        KernelObject* object = InputDevices::GetObject(deviceId);

        if (!object) {
          context.eax = 0;

          break;
        }

        UInt32 allowed = ABI::Devices::InputDevices::RightRead
          | ABI::Devices::InputDevices::RightControl
          | ABI::Devices::InputDevices::RightRegister;

        rights &= allowed;

        if (rights == 0) {
          context.eax = 0;

          break;
        }

        HandleTable::Handle handle = tcb->handleTable->Create(
          KernelObjectType::InputDevice,
          object,
          rights
        );

        context.eax = handle;

        break;
      }

      case SystemCall::IRQ_Register: {
        UInt32 irq = 0;
        UInt32 irqOrHandle = context.ebx;
        UInt32 portId = context.ecx;
        bool isHandle = HandleTable::IsHandle(irqOrHandle);

        if (!ResolveIRQHandle(
          irqOrHandle,
          ABI::IRQ::RightRegister,
          irq
        )) {
          context.eax = 1;

          break;
        }

        if (!isHandle && !Kernel::Task::IsCurrentTaskCoordinator()) {
          context.eax = 1;

          break;
        }

        bool ok = IRQ::Register(irq, portId);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::IRQ_Unregister: {
        UInt32 irq = 0;
        UInt32 irqOrHandle = context.ebx;
        bool isHandle = HandleTable::IsHandle(irqOrHandle);

        if (!ResolveIRQHandle(
          irqOrHandle,
          ABI::IRQ::RightUnregister,
          irq
        )) {
          context.eax = 1;

          break;
        }

        if (!isHandle && !Kernel::Task::IsCurrentTaskCoordinator()) {
          context.eax = 1;

          break;
        }

        bool ok = IRQ::Unregister(irq);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::IRQ_Enable: {
        UInt32 irq = 0;
        UInt32 irqOrHandle = context.ebx;
        bool isHandle = HandleTable::IsHandle(irqOrHandle);

        if (!ResolveIRQHandle(
          irqOrHandle,
          ABI::IRQ::RightEnable,
          irq
        )) {
          context.eax = 1;

          break;
        }

        if (!isHandle && !Kernel::Task::IsCurrentTaskCoordinator()) {
          context.eax = 1;

          break;
        }

        bool ok = IRQ::Enable(irq);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::IRQ_Disable: {
        UInt32 irq = 0;
        UInt32 irqOrHandle = context.ebx;
        bool isHandle = HandleTable::IsHandle(irqOrHandle);

        if (!ResolveIRQHandle(
          irqOrHandle,
          ABI::IRQ::RightDisable,
          irq
        )) {
          context.eax = 1;

          break;
        }

        if (!isHandle && !Kernel::Task::IsCurrentTaskCoordinator()) {
          context.eax = 1;

          break;
        }

        bool ok = IRQ::Disable(irq);

        context.eax = ok ? 0 : 1;

        break;
      }

      case SystemCall::IRQ_Open: {
        if (!Kernel::Task::IsCurrentTaskCoordinator()) {
          context.eax = 0;

          break;
        }

        UInt32 irq = context.ebx;
        UInt32 rights = context.ecx;

        if (rights == 0) {
          context.eax = 0;

          break;
        }

        Kernel::Task::ControlBlock* tcb = Kernel::Task::GetCurrent();

        if (!tcb || !tcb->handleTable) {
          context.eax = 0;

          break;
        }

        KernelObject* object = IRQ::GetObject(irq);

        if (!object) {
          context.eax = 0;

          break;
        }

        UInt32 allowed = ABI::IRQ::RightRegister
          | ABI::IRQ::RightUnregister
          | ABI::IRQ::RightEnable
          | ABI::IRQ::RightDisable;

        rights &= allowed;

        if (rights == 0) {
          context.eax = 0;

          break;
        }

        HandleTable::Handle handle = tcb->handleTable->Create(
          KernelObjectType::IRQLine,
          object,
          rights
        );

        context.eax = handle;

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
            UInt32 phys = PhysicalAllocator::AllocatePage(true);

            if (phys == 0) {
              ok = false;

              break;
            }

            Arch::AddressSpace::MapPage(
              addressSpace,
              vaddr,
              phys,
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
