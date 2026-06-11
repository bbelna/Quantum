/**
 * @file Include/Quantum/Kernel/ABI/ABITypes.hpp
 * @brief Declaration of the IPC ABI.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

#include "../Arch.hpp"
#include "../IPC.hpp"
#include "ABI.hpp"

extern "C" void* malloc(unsigned int size);
extern "C" void free(void* ptr);

/**
 * @brief ABI functions for Inter-Process Communication (IPC).
 */
namespace Quantum::Kernel::ABI::IPC {
  using Kernel::IPC::IPCPortID;
  using Kernel::IPC::IPCPortResourceID;
  using Kernel::IPC::IPCPortRights;
  using Kernel::IPC::IPCMessage;
  using Kernel::IPC::IPCMessageType;
  using Kernel::IPC::IPCSharedDescriptor;

  /**
   * @brief Copies a kernel-mapped IPC message into heap-allocated memory
   *        and unmaps the original kernel buffer.
   *
   * Kernel IPC receive syscalls return messages in kernel-mapped pages
   * that must be freed with the `Memory_Free` syscall, not with the
   * userspace heap's `free()`. This helper copies the message header
   * and payload into a single heap allocation so callers can manage
   * the lifetime with `free()` or `delete`.
   *
   * @param kernelMessage Pointer to the kernel-mapped IPCMessage, or
   *                      nullptr.
   * @return Heap-allocated copy of the message, or nullptr on failure.
   *         The caller must free the returned pointer with `free()` or
   *         `delete`.
   */
  inline IPCMessage* CopyToHeap(IPCMessage* kernelMessage) {
    if (!kernelMessage) return nullptr;

    // Snapshot all fields from the kernel buffer into locals before any
    // heap operations so the compiler cannot reorder reads past the
    // Memory_Free that unmaps the kernel pages.
    Concurrency::ProcessID sendingProcessID = kernelMessage->SendingProcessID;
    void* kernelPayload = kernelMessage->Payload;
    Size payloadSize = kernelMessage->PayloadSizeInBytes;
    IPC::IPCMessageType messageType = kernelMessage->Type;
    IPC::IPCSharedDescriptor sharedDescriptor = kernelMessage->SharedDescriptor;
    UInt32 kernelAddress = reinterpret_cast<UInt32>(kernelMessage);

    Size totalSize = sizeof(IPCMessage) + payloadSize;

    auto* buffer = static_cast<UInt8*>(
      malloc(static_cast<unsigned int>(totalSize))
    );

    if (!buffer) {
      Invoke(
        KernelOperation::Memory_Free, kernelAddress
      );

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

    Invoke(
      KernelOperation::Memory_Free, kernelAddress
    );

    return heapMessage;
  }

  /**
   * @brief Opens a handle/resource to an IPC endpoint.
   * @param portID The unique identifier of the IPC port to open.
   * @param rights The desired rights/permissions to associate with the opened
   *               IPC port handle/resource.
   * @return A handle/resource identifier for the opened IPC port, or -1
   *         if the operation fails (e.g., invalid port ID, insufficient
   *         permissions, etc.).
   */
  inline IPCPortResourceID Open(
    IPCPortID portID,
    IPCPortRights rights
  ) {
    return Invoke(
      KernelOperation::IPC_Open,
      portID,
      static_cast<UInt32>(rights)
    );
  }

  /**
   * @brief Opens a handle/resource to an IPC endpoint, returning the
   *        actual port ID assigned by the kernel.
   * @param portID The unique identifier of the IPC port to open. Pass
   *               `static_cast<IPCPortID>(-1)` to request the kernel to
   *               auto-assign a unique port ID.
   * @param rights The desired rights/permissions to associate with the opened
   *               IPC port handle/resource.
   * @param outPortID Pointer to receive the actual port ID. When
   *                  auto-assignment is requested, this will contain the
   *                  kernel-assigned port ID. Otherwise, it will contain the
   *                  original port ID.
   * @return A handle/resource identifier for the opened IPC port, or -1
   *         if the operation fails.
   */
  inline IPCPortResourceID Open(
    IPCPortID portID,
    IPCPortRights rights,
    IPCPortID* outPortID
  ) {
    UInt32 portArg = portID;
    UInt32 result;

    asm volatile(
      "movl %%esp, %%ecx\n"
      "leal 1f, %%edx\n"
      "sysenter\n"
      "1:"
      : "=a"(result), "+b"(portArg)
      : "0"(static_cast<UInt32>(KernelOperation::IPC_Open)),
        "S"(static_cast<UInt32>(rights))
      : "memory", "cc", "ecx", "edx", "edi"
    );

    if (outPortID)
      *outPortID = static_cast<IPCPortID>(portArg);

    return static_cast<IPCPortResourceID>(result);
  }

  /**
   * @brief Sends a message to an IPC endpoint.
   * @param handle The handle/resource identifier of the IPC port to send to.
   * @param message Pointer to the message data to send.
   * @param messageSize Size of the message data in bytes.
   */
  inline void Send(
    IPCPortResourceID handle,
    const void* message,
    Size messageSize
  ) {
    Invoke(
      KernelOperation::IPC_Send,
      handle,
      reinterpret_cast<UInt32>(message),
      messageSize
    );
  }

  /**
   * @brief Transfers a handle to an IPC endpoint.
   * @param handle The handle/resource identifier of the IPC port to send to.
   * @param handleToSend The handle/resource identifier to send.
   */
  inline bool Transfer(
    IPCPortResourceID handle,
    IPCPortResourceID handleToSend
  ) {
    return static_cast<bool>(
      Invoke(
        KernelOperation::IPC_Transfer,
        handle,
        handleToSend
      )
    );
  }

  /**
   * @brief Receives a message from an IPC endpoint.
   * @param handle The handle/resource identifier of the IPC port to receive
   *               from.
   * @param buffer Destination buffer for the received message.
   * @param bufferSize Size of the destination buffer in bytes.
   */
  inline IPCMessage* Receive(IPCPortResourceID handle) {
    auto* kernelMessage = reinterpret_cast<IPCMessage*>(
      Invoke(
        KernelOperation::IPC_Receive,
        handle
      )
    );

    return CopyToHeap(kernelMessage);
  }

  /**
   * @brief Tries to receive a message from an IPC endpoint without
   *        blocking. Returns immediately with null if no message is
   *        available.
   * @param handle The handle/resource identifier of the IPC port to
   *               receive from.
   * @return Pointer to the received IPCMessage, or nullptr if no
   *         message is available.
   */
  inline IPCMessage* TryReceive(IPCPortResourceID handle) {
    auto* kernelMessage = reinterpret_cast<IPCMessage*>(
      Invoke(
        KernelOperation::IPC_TryReceive,
        handle
      )
    );

    return CopyToHeap(kernelMessage);
  }

  /**
   * @brief Receives a message into a caller-provided buffer, avoiding
   *        per-message page allocation. The IPCMessage header and payload
   *        are copied directly into the supplied buffer.
   * @param handle The handle of the IPC port to receive from.
   * @param buffer Pointer to the destination buffer.
   * @param bufferSize Size of the buffer in bytes.
   * @return Pointer to the IPCMessage in the buffer, or nullptr if the
   *         message does not fit or on failure.
   */
  inline IPCMessage* ReceiveInto(
    IPCPortResourceID handle,
    void* buffer,
    Size bufferSize
  ) {
    return reinterpret_cast<IPCMessage*>(
      Invoke(
        KernelOperation::IPC_ReceiveInto,
        handle,
        reinterpret_cast<UInt32>(buffer),
        static_cast<UInt32>(bufferSize)
      )
    );
  }

  /**
   * @brief Blocks until a message arrives on any of the specified IPC
   *        ports. Returns the message and indicates which port received it.
   * @param handles Array of IPCPortResourceID handles to wait on.
   * @param count Number of handles (max 16).
   * @param outIndex Receives the index into `handles` of the port that
   *                 had the message.
   * @return Pointer to the received IPCMessage, or nullptr on failure.
   */
  inline IPCMessage* ReceiveAny(
    IPCPortResourceID* handles,
    Size count,
    Size* outIndex
  ) {
    UInt32 arg1 = reinterpret_cast<UInt32>(handles);
    UInt32 arg2 = static_cast<UInt32>(count);
    UInt32 arg3 = 0;
    UInt32 result;

    asm volatile(
      "movl %%esp, %%ecx\n"
      "leal 1f, %%edx\n"
      "sysenter\n"
      "1:"
      : "=a"(result), "+b"(arg1), "+S"(arg2), "+D"(arg3)
      : "0"(static_cast<UInt32>(KernelOperation::IPC_ReceiveAny))
      : "memory", "cc", "ecx", "edx"
    );

    if (outIndex) {
      *outIndex = static_cast<Size>(arg3);
    }

    return CopyToHeap(reinterpret_cast<IPCMessage*>(result));
  }

  /**
   * @brief Closes a handle/resource to an IPC endpoint.
   * @param handle The handle/resource identifier of the IPC port to close.
   */
  inline void Close(IPCPortResourceID handle) {
    Invoke(KernelOperation::IPC_Close, handle);
  }
}
