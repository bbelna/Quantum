/**
 * @file Include/Quantum/Clients/KernelClient.hpp
 * @brief Declares @ref @QClients::KernelClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/HAL.hpp>
#include <Quantum/Kernel/ABI.hpp>
#include <Quantum/Kernel.hpp>
#include <Quantum/Types.hpp>
#include <Quantum/Structures/List.hpp>

namespace Quantum::Clients {
  /**
   * @brief Client-side interface to the QuantumOS kernel.
   *
   * Provides a flat, unified API for all kernel system calls. Each method
   * maps to a single @ref KernelOperation and issues the corresponding
   * SYSENTER via @ref InvokeOS with subsystem 0.
   *
   * @code
   *   KernelClient kernel;
   *   auto bufferID = kernel.CreateSharedBuffer(4096);
   *   auto address  = kernel.AttachSharedBuffer(bufferID);
   *   kernel.DetachSharedBuffer(address);
   * @endcode
   */
  class KernelClient {
    public:
      /**
       * @brief Creates a new @ref KernelClient.
       */
      KernelClient() = default;

      /**
       * @brief Exits the current process with the given status code.
       * @param statusCode The exit status code.
       */
      void ExitProcess(UInt32 statusCode);

      /**
       * @brief Spawns a new process.
       * @param name The process name.
       * @param entryPoint Virtual address of the entry point.
       * @param parameters Optional spawn parameters for binary mapping, or
       *                   `nullptr` for no mapping.
       * @return The new process ID on success, or -1 on failure.
       */
      ProcessID SpawnProcess(
        const char* name,
        UIntPtr entryPoint,
        const ProcessSpawnParameters* parameters = nullptr
      );

      /**
       * @brief Requests a specific permission for the current process.
       * @param permission The permission to request.
       * @return `true` if the permission was granted.
       */
      bool RequestPermission(ProcessPermissions permission);

      /**
       * @brief Gets the process ID of the current process.
       * @return The current process ID.
       */
      ProcessID GetProcessID();

      /**
       * @brief Gets a list of all running processes.
       * @param buffer Pointer to an array of ProcessInfo structures to fill.
       * @param maxCount Maximum number of entries to write.
       * @return The number of entries written.
       */
      UInt32 GetProcessList(
        ProcessInfo* buffer,
        UInt32 maxCount
      );

      /**
       * @brief Waits for a process to exit.
       * @param processID The process ID to wait for.
       * @return The exit code of the process, or -1 on failure.
       */
      Int32 WaitForProcess(ProcessID processID);

      /**
       * @brief Non-blocking check whether a process is still alive.
       * @param processID The process ID to check.
       * @return `true` if the process exists and has not yet exited.
       */
      bool IsProcessAlive(ProcessID processID);

      /**
       * @brief Writes a formatted message to the kernel log.
       * @param level The log level.
       * @param message Null-terminated format string.
       * @param ... Variable arguments for formatting.
       */
      void WriteLog(LogLevel level, const char* message, ...);

      /**
       * @brief Sets the kernel log level threshold.
       * @param level The new log level.
       */
      void SetLogLevel(LogLevel level);

      /**
       * @brief Allocates memory in the calling process' address space.
       * @param sizeInBytes The size of the block to allocate.
       * @return The virtual address, or null on failure.
       */
      UIntPtr AllocateBlock(Size sizeInBytes);

      /**
       * @brief Frees a previously allocated memory block.
       * @param address The virtual address to free.
       */
      void FreeBlock(UIntPtr address);

      /**
       * @brief Reserves a contiguous virtual address range with lazy
       *        allocation via demand paging.
       * @param sizeInBytes The size of the range to reserve.
       * @return The virtual address, or null on failure.
       */
      UIntPtr AllocateRange(Size sizeInBytes);

      /**
       * @brief Gets information about physical memory usage.
       * @param info Pointer to a MemoryInfo structure to fill.
       * @return `true` on success.
       */
      bool GetMemoryInfo(Quantum::Kernel::ABI::KernelMemoryInfo* info);

      /**
       * @brief Creates a shared memory buffer and maps it into the caller's
       *        address space.
       * @param sizeInBytes The size of the buffer.
       * @return The SharedBufferID on success, or 0 on failure.
       */
      SharedBufferID CreateSharedBuffer(Size sizeInBytes);

      /**
       * @brief Maps an existing shared buffer into the caller's address
       *        space.
       * @param bufferID The SharedBufferID to attach.
       * @return The virtual address of the mapping, or null on failure.
       */
      UIntPtr AttachSharedBuffer(SharedBufferID bufferID);

      /**
       * @brief Unmaps a shared buffer from the caller's address space.
       * @param address The virtual address of the attached mapping.
       */
      void DetachSharedBuffer(UIntPtr address);

      /**
       * @brief Translates a virtual address to its physical address.
       * @param address The virtual address to translate.
       * @return The physical address, or 0 if unmapped.
       */
      UInt32 VirtualToPhysical(UIntPtr address);

      /**
       * @brief Allocates memory below a physical address limit for DMA.
       * @param sizeInBytes The size of the block to allocate.
       * @param physicalLimit Exclusive upper bound on the physical address.
       * @return The virtual address, or null on failure.
       */
      UIntPtr AllocateDMA(Size sizeInBytes, UInt32 physicalLimit);

      /**
       * @brief Gets memory pressure diagnostics.
       * @param info Pointer to a MemoryPressureInfo structure to fill.
       * @return `true` on success.
       */
      bool GetMemoryPressureInfo(Quantum::Kernel::ABI::KernelMemoryPressureInfo* info);

      /**
       * @brief Gets per-tag kernel memory block counts.
       * @param stats Pointer to a MemoryTagStats structure to fill.
       * @return `true` on success.
       */
      bool GetMemoryTagStats(Quantum::Kernel::ABI::KernelMemoryTagStats* stats);

      /**
       * @brief Releases the initial image (startup bundle) memory reserved
       *        at boot, returning those physical pages to the free pool.
       *
       * Should be called exactly once after the startup server has finished
       * spawning all boot entries. Subsequent calls are safe no-ops.
       */
      void FreeInitialImage();

      /**
       * @brief Blocks until the specified IPC port exists.
       *
       * If the port is already registered, returns immediately.
       *
       * @param portID The IPC port ID to wait for.
       * @return `true` if the port now exists.
       */
      bool WaitForIPCPort(IPCPortID portID);

      /**
       * @brief Opens a handle to an IPC port.
       * @param portID The IPC port ID to open.
       * @param rights The desired rights for the port handle.
       * @return A handle to the port, or -1 on failure.
       */
      IPCPortHandle OpenIPCPort(
        IPCPortID portID,
        IPCPortRights rights
      );

      /**
       * @brief Opens a handle to an IPC port, receiving the actual assigned
       *        port ID. Pass `static_cast<IPCPortID>(-1)` to auto-assign.
       * @param portID The IPC port ID to open, or -1 for auto-assignment.
       * @param rights The desired rights for the port handle.
       * @param outPortID Pointer to receive the actual port ID.
       * @return A handle to the port, or -1 on failure.
       */
      IPCPortHandle OpenIPCPort(
        IPCPortID portID,
        IPCPortRights rights,
        IPCPortID* outPortID
      );

      /**
       * @brief Sends a message to an IPC port.
       * @param handle The port handle to send to.
       * @param message Pointer to the message data.
       * @param messageSize Size of the message in bytes.
       */
      void SendIPCMessage(
        IPCPortHandle handle,
        const void* message,
        Size messageSize
      );

      /**
       * @brief Transfers a handle to another process via an IPC port.
       * @param handle The port handle to send through.
       * @param handleToSend The handle to transfer.
       * @return `true` on success.
       */
      bool TransferIPCHandle(
        IPCPortHandle handle,
        IPCPortHandle handleToSend
      );

      /**
       * @brief Receives a message from an IPC port (blocking).
       * @param handle The port handle to receive from.
       * @return Pointer to the received message, or nullptr on failure.
       */
      IPCMessage* ReceiveIPCMessage(
        IPCPortHandle handle
      );

      /**
       * @brief Tries to receive a message without blocking.
       * @param handle The port handle to receive from.
       * @return Pointer to the received message, or nullptr if none
       *         available.
       */
      IPCMessage* TryReceiveIPCMessage(IPCPortHandle handle);

      /**
       * @brief Receives a message into a caller-provided buffer, avoiding
       *        per-message page allocation.
       * @param handle The port handle to receive from.
       * @param buffer Pointer to the destination buffer.
       * @param bufferSize Size of the buffer in bytes.
       * @return Pointer to the IPCMessage in the buffer, or nullptr on
       *         failure.
       */
      IPCMessage* ReceiveIPCMessageInto(
        IPCPortHandle handle,
        void* buffer,
        Size bufferSize
      );

      /**
       * @brief Blocks until a message arrives on any of the specified ports.
       * @param handles Array of port handles to wait on.
       * @param count Number of handles (max 16).
       * @param outIndex Receives the index of the port that had the message.
       * @return Pointer to the received message, or nullptr on failure.
       */
      IPCMessage* ReceiveAnyIPCMessage(
        IPCPortHandle* handles,
        Size count,
        Size* outIndex
      );

      /**
       * @brief Sends a shared-buffer descriptor to an IPC port (zero-copy).
       *
       * The caller must have the shared buffer attached. The receiver reads
       * data directly from its own mapping of the shared buffer.
       *
       * @param handle The port handle to send to.
       * @param bufferID The SharedBufferID of the attached shared buffer.
       * @param offset Byte offset into the shared buffer where data begins.
       * @param size Number of bytes of data starting at @p offset.
       * @return `true` on success; `false` on failure.
       */
      bool SendSharedIPCMessage(
        IPCPortHandle handle,
        SharedBufferID bufferID,
        UInt32 offset,
        UInt32 size
      );

      /**
       * @brief Checks whether a received IPC message is a shared-buffer
       *        (zero-copy) message.
       * @param message Pointer to the received IPCMessage.
       * @return `true` if the message carries a shared-buffer descriptor;
       *         `false` if it carries a traditional payload.
       */
      static bool IsSharedIPCMessage(
        const IPCMessage* message
      );

      /**
       * @brief Closes a handle to an IPC port.
       * @param handle The port handle to close.
       */
      void CloseIPCPort(IPCPortHandle handle);

      /**
       * @brief Gets scheduler performance statistics.
       * @param stats Pointer to a SchedulerStats structure to fill.
       * @return `true` on success.
       */
      bool GetSchedulerStats(
        Quantum::Kernel::Concurrency::SchedulerStats* stats
      );

      /**
       * @brief Yields the current thread's time slice to the scheduler.
       */
      void YieldThread();

      /**
       * @brief Puts the calling thread to sleep.
       * @param ticks Number of timer ticks to sleep.
       */
      void SleepThread(UInt32 ticks);

      /**
       * @brief Creates a new thread in the calling process.
       * @param entryPoint User-space entry point address.
       * @param argument Argument passed to the entry point.
       * @return Thread ID on success, or 0 on failure.
       */
      UInt32 CreateThread(UInt32 entryPoint, UInt32 argument);

      /**
       * @brief Terminates the calling thread. If this is the last thread
       *        in the process, the process is terminated with the given
       *        exit code.
       * @param exitCode Exit code for the thread.
       */
      [[noreturn]] void ExitThread(Int32 exitCode);

      /**
       * @brief Atomically checks `*address == expected` and suspends the
       *        calling thread until woken by @ref FutexWake.
       * @param address Pointer to a UInt32 in user memory.
       * @param expected The value to compare against.
       * @return 0 if woken by FutexWake, 1 if the value did not match.
       */
      UInt32 FutexWait(volatile UInt32* address, UInt32 expected);

      /**
       * @brief Wakes up to @p count threads blocked in @ref FutexWait on
       *        @p address.
       * @param address Pointer to the same UInt32 passed to FutexWait.
       * @param count Maximum number of threads to wake.
       * @return Number of threads actually woken.
       */
      UInt32 FutexWake(volatile UInt32* address, UInt32 count);

      /**
       * @brief Sets the scheduling priority of the calling thread.
       * @param priority 0-15 for time-sharing, 16-31 for real-time.
       * @return 0 on success, 1 on invalid priority.
       */
      UInt32 SetThreadPriority(UInt32 priority);

      /**
       * @brief Reads a byte from an I/O port.
       * @param port The I/O port address.
       * @return The byte read.
       */
      UInt8 PortIn8(UInt16 port);

      /**
       * @brief Reads a word from an I/O port.
       * @param port The I/O port address.
       * @return The word read.
       */
      UInt16 PortIn16(UInt16 port);

      /**
       * @brief Reads a double word from an I/O port.
       * @param port The I/O port address.
       * @return The double word read.
       */
      UInt32 PortIn32(UInt16 port);

      /**
       * @brief Writes a byte to an I/O port.
       * @param port The I/O port address.
       * @param value The byte to write.
       */
      void PortOut8(UInt16 port, UInt8 value);

      /**
       * @brief Writes a word to an I/O port.
       * @param port The I/O port address.
       * @param value The word to write.
       */
      void PortOut16(UInt16 port, UInt16 value);

      /**
       * @brief Writes a double word to an I/O port.
       * @param port The I/O port address.
       * @param value The double word to write.
       */
      void PortOut32(UInt16 port, UInt32 value);

      /**
       * @brief Claims a hardware IRQ for the calling process.
       * @param irq The IRQ number to claim (0-15).
       * @return A resource handle for the interrupt, or -1 on failure.
       */
      Kernel::Resources::ResourceID ClaimInterrupt(UInt8 irq);

      /**
       * @brief Waits for the next interrupt on a claimed IRQ.
       * @param handle The resource handle returned by @ref ClaimInterrupt.
       */
      void WaitForInterrupt(Kernel::Resources::ResourceID handle);

      /**
       * @brief Releases a previously claimed hardware IRQ.
       * @param handle The resource handle returned by @ref ClaimInterrupt.
       */
      void ReleaseInterrupt(Kernel::Resources::ResourceID handle);

      /**
       * @brief Gets a list of platform devices.
       * @return A pointer list of platform devices.
       */
      Structures::Lists::PointerList<HAL::Device> GetDevices();

      /**
       * @brief Invokes a platform driver operation.
       * @param deviceID The ID of the target device.
       * @param operation The driver-specific operation code.
       * @param payload Pointer to a driver-specific payload structure.
       * @return The result of the driver operation.
       */
      UInt32 InvokeDriver(
        UInt32 deviceID,
        UInt32 operation,
        void* payload
      );

    private:
      /**
       * @brief Copies a kernel-mapped IPC message to a heap-allocated buffer
       *        and frees the kernel memory via @ref FreeMemory.
       * @param kernelMessage Pointer to the kernel-mapped IPCMessage, or
       *                      `nullptr`.
       * @return Heap-allocated copy of the message, or `nullptr` on failure.
       *         The caller must free the returned pointer with `free()`.
       */
      IPCMessage* CopyIPCMessageToHeap(
        IPCMessage* kernelMessage
      );
  };
}
