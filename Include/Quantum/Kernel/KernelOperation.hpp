/**
 * @file Include/Quantum/Kernel/KernelOperation.hpp
 * @brief Declares @ref @QKrnl::KernelOperation.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

namespace Quantum::Kernel {
  /**
   * @brief Enumerates kernel ABI operations.
   */
  enum class KernelOperation : UInt32 {
    /**
     * @brief Exit the current @ref Process with a given exit code.
     */
    Process_Exit = 0,

    /**
     * @brief Spawn a new @ref Process with the given parameters.
     */
    Process_Spawn = 1,

    /**
     * @brief Requests a @ref ProcessPermissions for the current @ref Process.
     */
    Process_RequestPermission = 2,

    /**
     * @brief Get the @ref ProcessID of the current @ref Process.
     */
    Process_GetID = 3,

    /**
     * @brief Get a list of all running @ref Process instances.
     */
    Process_GetList = 4,

    /**
     * @brief Wait for a @ref Process to exit.
     */
    Process_Wait = 5,

    /**
     * @brief Non-blocking check whether a process is still alive.
     *
     * Returns `1` if the process exists and has not yet reached
     * @ref ProcessState::Zombie, `0` if it has exited or was not found.
     */
    Process_IsAlive = 6,

    /** 
     * @brief Write a message to the kernel log.
     */
    Log_Write = 16,

    /**
     * @brief Set the kernel's log level.
     */
    Log_SetLevel = 17,

    /**
     * @brief Get a list of devices enumerated by the kernel drivers.
     */
    Drivers_GetDevices = 20,

    /**
     * @brief Invoke a kernel driver operation.
     */
    Drivers_Invoke = 21,

    /**
     * @brief Allocates a @ref MemoryBlock.
     */
    Memory_Allocate = 32,

    /**
     * @brief Frees a @ref MemoryBlock.
     */
    Memory_Free = 33,

    /**
     * @brief Reserves a contiguous address range in the calling @ref Process
     *        address space.
     */
    Memory_AllocateRange = 34,

    /**
     * @brief Gets information about the system's physical memory usage.
     */
    Memory_GetInfo = 35,

    /**
     * @brief Allocates a shared memory buffer.
     */
    Memory_CreateShared = 36,

    /**
     * @brief Maps an existing shared buffer into the caller's address space.
     */
    Memory_AttachShared = 37,

    /**
     * @brief Unmaps a shared buffer from the caller's address space.
     */
    Memory_DetachShared = 38,

    /**
     * @brief Translates a virtual address to its underlying physical address.
     */
    Memory_VirtualToPhysical = 39,

    /**
     * @brief
     *   Allocates a @ref MemoryBlock whose physical address is below a
     *   given limit and maps it into the caller's address space.
     *
     * Used by drivers that program DMA controllers with restricted address
     * ranges (e.g. ISA DMA requires addresses below 16 MB).
     */
    Memory_AllocateDMA = 40,

    /**
     * @brief Gets @ref MemoryPressureInfo.
     */
    Memory_GetPressureInfo = 41,

    /**
     * @brief Opens an @ref IPCPort and a corresponding @ref IPCPortResource.
     */
    IPC_Open = 64,

    /**
     * @brief Closes an @ref IPCPortResource.
     */
    IPC_Close = 65,

    /**
     * @brief Sends an @ref IPCMessage to a @ref Process.
     */
    IPC_Send = 66,

    /**
     * @brief Receives an @ref IPCMessage from an @ref IPCPortResource.
     */
    IPC_Receive = 67,

    /**
     * @brief Transfers an @ref IPCPortResource to another @ref Process.
     */
    IPC_Transfer = 68,

    /**
     * @brief Non-blocking variant of @ref IPC_Receive.
     * @note Returns immediately with `0` if no messages are available.
     */
    IPC_TryReceive = 69,

    /**
     * @brief Blocks until an @ref IPCMessage arrives on any of the specified
     *        @ref IPCPort.
     */
    IPC_ReceiveAny = 70,

    /**
     * @brief Receives an @ref IPCMessage into a caller-provided buffer.
     */
    IPC_ReceiveInto = 71,

    /**
     * @brief Sends an @ref IPCMessage with an attached shared buffer.
     * @note The sender must have the shared buffer attached.
     *       The receiver reads data directly from its own mapping.
     */
    IPC_SendShared = 72,

    /**
     * @brief
     *   Blocks the calling @ref Thread until an @ref IPCPort with the specified
     *   @ref IPCPortID exists.
     *
     * If the @ref IPCPort is already registered, returns immediately.
     * Otherwise, the @ref Thread sleeps until another @ref Process creates the
     * @ref IPCPort via @ref IPC_Open with @ref IPCPortRights::Manage.
     */
    IPC_WaitForPort = 73,

    /**
     * @brief Yields the current @ref Thread time slice to the scheduler.
     */
    Thread_Yield = 80,

    /**
     * @brief Puts the calling @ref Thread to sleep for a specified number of
     *        timer ticks.
     */
    Thread_Sleep = 81,

    /**
     * @brief Creates a new @ref Thread in the calling @ref Process.
     */
    Thread_Create = 82,

    /**
     * @brief
     *   Atomically checks if value at the specified address equals an expected
     *   value.
     * @note
     *   If so, suspends the calling @ref Thread until another @ref Thread calls
     *   @ref Thread_FutexWake on the same address.
     */
    Thread_FutexWait = 83,

    /**
     * @brief
     *   Wakes up to \f$N\f$ @ref Thread that are blocked in
     *   @ref Thread_FutexWait on the given address.
     */
    Thread_FutexWake = 84,

    /**
     * @brief Sets the @ref ThreadPriority of the calling @ref Thread.
     */
    Thread_SetPriority = 85,

    /**
     * @brief Terminates the calling @ref Thread.
     *
     * The @ref Thread resources are cleaned up by the kernel. If this is the
     * last @ref Thread in the process, the @ref Process is terminated with exit
     * code `0`.
     */
    Thread_Exit = 86,

    /**
     * @brief Inputs a byte (@ref UInt8) from the specified I/O port.
     */
    PortIO_In8 = 96,

    /**
     * @brief Inputs a word (@ref UInt16) from the specified I/O port.
     */
    PortIO_In16 = 97,

    /**
     * @brief Inputs a double word (@ref UInt32) from the specified I/O port.
     */
    PortIO_In32 = 98,

    /**
     * @brief Outputs a byte (@ref UInt8) to the specified I/O port.
     */
    PortIO_Out8 = 99,

    /**
     * @brief Outputs a word (@ref UInt16) to the specified I/O port.
     */
    PortIO_Out16 = 100,

    /**
     * @brief Outputs a double word (@ref UInt32) to the specified I/O port.
     */
    PortIO_Out32 = 101,

    /**
     * @brief Claims an IRQ for the calling @ref Process.
     */
    Interrupt_Claim = 112,

    /**
     * @brief Waits for the next interrupt on a claimed IRQ.
     */
    Interrupt_Wait = 113,

    /**
     * @brief Releases a previously claimed IRQ.
     */
    Interrupt_Release = 114,

    /**
     * @brief Retrieves per-tag kernel @ref MemoryBlock counts.
     */
    Memory_GetTagStats = 120,

    /**
     * @brief Retrieves scheduler performance statistics.
     */
    Scheduler_GetStats = 128,

    /**
     * @brief Frees the reserved @ref MemoryBlock for the initial image.
     * @note Should be called exactly once.
     */
    Memory_FreeInitialImage = 42
  };
}
