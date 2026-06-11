/**
 * @file Clients/KernelClient.cpp
 * @brief Implements @ref @QClients::KernelClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ClientsTypes.hpp>

#include "KernelClient.hpp"

namespace Quantum::Clients {
  void KernelClient::ExitProcess(UInt32 statusCode) {
    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Process_Exit),
      { statusCode }
    );
  }

  ProcessID KernelClient::SpawnProcess(
    const char* name,
    UIntPtr entryPoint,
    const ProcessSpawnParameters* parameters
  ) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Process_Spawn),
      {
        reinterpret_cast<UInt32>(name),
        entryPoint,
        reinterpret_cast<UInt32>(parameters)
      }
    );
  }

  bool KernelClient::RequestPermission(ProcessPermissions permission) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Process_RequestPermission),
      { static_cast<UInt32>(permission) }
    ) != 0;
  }

  ProcessID KernelClient::GetProcessID() {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Process_GetID),
      {}
    );
  }

  UInt32 KernelClient::GetProcessList(
    ProcessInfo* buffer,
    UInt32 maxCount
  ) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Process_GetList),
      { reinterpret_cast<UInt32>(buffer), maxCount }
    );
  }

  Int32 KernelClient::WaitForProcess(ProcessID processID) {
    return static_cast<Int32>(
      InvokeOS<UInt32, KernelOperationPayload>(
        0,
        static_cast<UInt32>(KernelOperation::Process_Wait),
        { static_cast<UInt32>(processID) }
      )
    );
  }

  bool KernelClient::IsProcessAlive(ProcessID processID) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Process_IsAlive),
      { static_cast<UInt32>(processID) }
    ) != 0;
  }

  void KernelClient::WriteLog(
    LogLevel level,
    const char* message,
    ...
  ) {
    if (!message) return;

    VariableArgumentsList args;
    char buffer[1024] = {};

    VARIABLE_ARGUMENTS_START(args, message);
    CString::FormatV(buffer, 1024, message, args);
    VARIABLE_ARGUMENTS_END(args);

    UInt32 length = CString::Length(buffer);

    if (length == 0) return;

    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Log_Write),
      {
        static_cast<UInt32>(level),
        reinterpret_cast<UInt32>(buffer),
        length
      }
    );
  }

  void KernelClient::SetLogLevel(LogLevel level) {
    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Log_SetLevel),
      { static_cast<UInt32>(level) }
    );
  }

  UIntPtr KernelClient::AllocateBlock(Size sizeInBytes) {
    return static_cast<UIntPtr>(
      InvokeOS<UInt32, KernelOperationPayload>(
        0,
        static_cast<UInt32>(KernelOperation::Memory_Allocate),
        { sizeInBytes }
      )
    );
  }

  void KernelClient::FreeBlock(UIntPtr address) {
    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Memory_Free),
      { address }
    );
  }

  UIntPtr KernelClient::AllocateRange(Size sizeInBytes) {
    return static_cast<UIntPtr>(
      InvokeOS<UInt32, KernelOperationPayload>(
        0,
        static_cast<UInt32>(KernelOperation::Memory_AllocateRange),
        { sizeInBytes }
      )
    );
  }

  bool KernelClient::GetMemoryInfo(KernelMemoryInfo* info) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Memory_GetInfo),
      { reinterpret_cast<UInt32>(info) }
    ) != 0;
  }

  SharedBufferID KernelClient::CreateSharedBuffer(Size sizeInBytes) {
    return static_cast<SharedBufferID>(
      InvokeOS<UInt32, KernelOperationPayload>(
        0,
        static_cast<UInt32>(KernelOperation::Memory_CreateShared),
        { sizeInBytes }
      )
    );
  }

  UIntPtr KernelClient::AttachSharedBuffer(SharedBufferID bufferID) {
    return static_cast<UIntPtr>(
      InvokeOS<UInt32, KernelOperationPayload>(
        0,
        static_cast<UInt32>(KernelOperation::Memory_AttachShared),
        { static_cast<UInt32>(bufferID) }
      )
    );
  }

  void KernelClient::DetachSharedBuffer(UIntPtr address) {
    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Memory_DetachShared),
      { address }
    );
  }

  UInt32 KernelClient::VirtualToPhysical(UIntPtr address) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Memory_VirtualToPhysical),
      { address }
    );
  }

  UIntPtr KernelClient::AllocateDMA(Size sizeInBytes, UInt32 physicalLimit) {
    return static_cast<UIntPtr>(
      InvokeOS<UInt32, KernelOperationPayload>(
        0,
        static_cast<UInt32>(KernelOperation::Memory_AllocateDMA),
        { sizeInBytes, physicalLimit }
      )
    );
  }

  bool KernelClient::GetMemoryPressureInfo(KernelMemoryPressureInfo* info) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Memory_GetPressureInfo),
      { reinterpret_cast<UInt32>(info) }
    ) != 0;
  }

  bool KernelClient::GetMemoryTagStats(KernelMemoryTagStats* stats) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Memory_GetTagStats),
      { reinterpret_cast<UInt32>(stats) }
    ) != 0;
  }

  bool KernelClient::GetSchedulerStats(SchedulerStats* stats) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Scheduler_GetStats),
      { reinterpret_cast<UInt32>(stats) }
    ) != 0;
  }

  void KernelClient::FreeInitialImage() {
    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Memory_FreeInitialImage),
      {}
    );
  }

  bool KernelClient::WaitForIPCPort(IPCPortID portID) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::IPC_WaitForPort),
      { static_cast<UInt32>(portID) }
    ) != 0;
  }

  IPCPortHandle KernelClient::OpenIPCPort(
    IPCPortID portID,
    IPCPortRights rights
  ) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::IPC_Open),
      { portID, static_cast<UInt32>(rights) }
    );
  }

  IPCPortHandle KernelClient::OpenIPCPort(
    IPCPortID portID,
    IPCPortRights rights,
    IPCPortID* outPortID
  ) {
    auto syscallResult = InvokeOSEx(
      static_cast<UInt32>(KernelOperation::IPC_Open),
      { portID, static_cast<UInt32>(rights) }
    );

    if (outPortID) {
      *outPortID = static_cast<IPCPortID>(syscallResult.Arg1);
    }

    return static_cast<IPCPortHandle>(syscallResult.Result);
  }

  void KernelClient::SendIPCMessage(
    IPCPortHandle handle,
    const void* message,
    Size messageSize
  ) {
    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::IPC_Send),
      {
        handle,
        reinterpret_cast<UInt32>(message),
        messageSize
      }
    );
  }

  bool KernelClient::TransferIPCHandle(
    IPCPortHandle handle,
    IPCPortHandle handleToSend
  ) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::IPC_Transfer),
      { handle, handleToSend }
    ) != 0;
  }

  IPCMessage* KernelClient::ReceiveIPCMessage(IPCPortHandle handle) {
    auto* kernelMessage = reinterpret_cast<IPCMessage*>(
      InvokeOS<UInt32, KernelOperationPayload>(
        0,
        static_cast<UInt32>(KernelOperation::IPC_Receive),
        { handle }
      )
    );

    return CopyIPCMessageToHeap(kernelMessage);
  }

  IPCMessage* KernelClient::TryReceiveIPCMessage(IPCPortHandle handle) {
    auto* kernelMessage = reinterpret_cast<IPCMessage*>(
      InvokeOS<UInt32, KernelOperationPayload>(
        0,
        static_cast<UInt32>(KernelOperation::IPC_TryReceive),
        { handle }
      )
    );

    return CopyIPCMessageToHeap(kernelMessage);
  }

  IPCMessage* KernelClient::ReceiveIPCMessageInto(
    IPCPortHandle handle,
    void* buffer,
    Size bufferSize
  ) {
    return reinterpret_cast<IPCMessage*>(
      InvokeOS<UInt32, KernelOperationPayload>(
        0,
        static_cast<UInt32>(KernelOperation::IPC_ReceiveInto),
        {
          handle,
          reinterpret_cast<UInt32>(buffer),
          static_cast<UInt32>(bufferSize)
        }
      )
    );
  }

  IPCMessage* KernelClient::ReceiveAnyIPCMessage(
    IPCPortHandle* handles,
    Size count,
    Size* outIndex
  ) {
    auto syscallResult = InvokeOSEx(
      static_cast<UInt32>(KernelOperation::IPC_ReceiveAny),
      {
        reinterpret_cast<UInt32>(handles),
        static_cast<UInt32>(count),
        0
      }
    );

    if (outIndex) {
      *outIndex = static_cast<Size>(syscallResult.Arg3);
    }

    return CopyIPCMessageToHeap(
      reinterpret_cast<IPCMessage*>(syscallResult.Result)
    );
  }

  bool KernelClient::SendSharedIPCMessage(
    IPCPortHandle handle,
    SharedBufferID bufferID,
    UInt32 offset,
    UInt32 size
  ) {
    IPCSharedDescriptor descriptor;
    descriptor.BufferID = bufferID;
    descriptor.Offset = offset;
    descriptor.Size = size;

    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::IPC_SendShared),
      {
        handle,
        reinterpret_cast<UInt32>(&descriptor)
      }
    ) != 0;
  }

  bool KernelClient::IsSharedIPCMessage(const IPCMessage* message) {
    return message && message->Type == IPCMessageType::Shared;
  }

  void KernelClient::CloseIPCPort(IPCPortHandle handle) {
    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::IPC_Close),
      { handle }
    );
  }

  void KernelClient::YieldThread() {
    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Thread_Yield),
      {}
    );
  }

  void KernelClient::SleepThread(UInt32 ticks) {
    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Thread_Sleep),
      { ticks }
    );
  }

  UInt32 KernelClient::CreateThread(UInt32 entryPoint, UInt32 argument) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Thread_Create),
      { entryPoint, argument }
    );
  }

  void KernelClient::ExitThread(Int32 exitCode) {
    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Thread_Exit),
      { static_cast<UInt32>(exitCode) }
    );

    for (;;) {}
  }

  UInt32 KernelClient::FutexWait(volatile UInt32* address, UInt32 expected) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Thread_FutexWait),
      { reinterpret_cast<UInt32>(address), expected }
    );
  }

  UInt32 KernelClient::FutexWake(volatile UInt32* address, UInt32 count) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Thread_FutexWake),
      { reinterpret_cast<UInt32>(address), count }
    );
  }

  UInt32 KernelClient::SetThreadPriority(UInt32 priority) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Thread_SetPriority),
      { priority }
    );
  }

  UInt8 KernelClient::PortIn8(UInt16 port) {
    return static_cast<UInt8>(
      InvokeOS<UInt32, KernelOperationPayload>(
        0,
        static_cast<UInt32>(KernelOperation::PortIO_In8),
        { static_cast<UInt32>(port) }
      )
    );
  }

  UInt16 KernelClient::PortIn16(UInt16 port) {
    return static_cast<UInt16>(
      InvokeOS<UInt32, KernelOperationPayload>(
        0,
        static_cast<UInt32>(KernelOperation::PortIO_In16),
        { static_cast<UInt32>(port) }
      )
    );
  }

  UInt32 KernelClient::PortIn32(UInt16 port) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::PortIO_In32),
      { static_cast<UInt32>(port) }
    );
  }

  void KernelClient::PortOut8(UInt16 port, UInt8 value) {
    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::PortIO_Out8),
      { static_cast<UInt32>(port), static_cast<UInt32>(value) }
    );
  }

  void KernelClient::PortOut16(UInt16 port, UInt16 value) {
    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::PortIO_Out16),
      { static_cast<UInt32>(port), static_cast<UInt32>(value) }
    );
  }

  void KernelClient::PortOut32(UInt16 port, UInt32 value) {
    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::PortIO_Out32),
      { static_cast<UInt32>(port), value }
    );
  }

  ResourceID KernelClient::ClaimInterrupt(UInt8 irq) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Interrupt_Claim),
      { static_cast<UInt32>(irq) }
    );
  }

  void KernelClient::WaitForInterrupt(ResourceID handle) {
    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Interrupt_Wait),
      { handle }
    );
  }

  void KernelClient::ReleaseInterrupt(ResourceID handle) {
    InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Interrupt_Release),
      { handle }
    );
  }

  PointerList<Device> KernelClient::GetDevices() {
    auto syscallResult = InvokeOSEx(
      static_cast<UInt32>(KernelOperation::Drivers_GetDevices),
      {}
    );

    return PointerList<Device>(
      reinterpret_cast<Device*>(syscallResult.Result),
      syscallResult.Arg1
    );
  }

  UInt32 KernelClient::InvokeDriver(
    UInt32 deviceID,
    UInt32 operation,
    void* payload
  ) {
    return InvokeOS<UInt32, KernelOperationPayload>(
      0,
      static_cast<UInt32>(KernelOperation::Drivers_Invoke),
      {
        deviceID,
        operation,
        reinterpret_cast<UInt32>(payload)
      }
    );
  }

  IPCMessage* KernelClient::CopyIPCMessageToHeap(
    IPCMessage* kernelMessage
  ) {
    if (!kernelMessage) return nullptr;

    // Snapshot all fields from the kernel buffer into locals before any
    // heap operations so the compiler cannot reorder reads past the
    // FreeMemory that unmaps the kernel pages.
    Concurrency::ProcessID sendingProcessID = kernelMessage->SendingProcessID;
    void* kernelPayload = kernelMessage->Payload;
    Size payloadSize = kernelMessage->PayloadSizeInBytes;
    IPCMessageType messageType = kernelMessage->Type;
    IPCSharedDescriptor sharedDescriptor = kernelMessage->SharedDescriptor;
    UInt32 kernelAddress = reinterpret_cast<UInt32>(kernelMessage);

    Size totalSize = sizeof(IPCMessage) + payloadSize;

    auto* buffer = static_cast<UInt8*>(
      malloc(static_cast<unsigned int>(totalSize))
    );

    if (!buffer) {
      FreeBlock(kernelAddress);
      return nullptr;
    }

    auto* heapMessage = reinterpret_cast<IPCMessage*>(buffer);

    heapMessage->SendingProcessID = sendingProcessID;
    heapMessage->PayloadSizeInBytes = payloadSize;
    heapMessage->Type = messageType;
    heapMessage->SharedDescriptor = sharedDescriptor;
    heapMessage->Next = nullptr;
    heapMessage->Previous = nullptr;

    if (payloadSize > 0 && kernelPayload) {
      void* heapPayload = buffer + sizeof(IPCMessage);

      Quantum::Core::Byte::Copy(heapPayload, kernelPayload, payloadSize);

      heapMessage->Payload = heapPayload;
    } else {
      heapMessage->Payload = nullptr;
    }

    FreeBlock(kernelAddress);

    return heapMessage;
  }
}
