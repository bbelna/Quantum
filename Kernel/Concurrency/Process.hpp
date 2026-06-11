/**
 * @file Kernel/Concurrency/Process.hpp
 * @brief Declares @ref @QKrnl::Concurrency::Process.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>
#include <Memory/AddressSpaceMap.hpp>

#include "ProcessPermissions.hpp"
#include "ProcessState.hpp"
#include "Thread.hpp"

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Process control block containing all per-process state.
   *
   * Allocated from the kernel heap by @ref ProcessManager::Create. Linked
   * into the global process list via @ref Process::Next /
   * @ref Process::Previous (intrusive doubly-linked list) and into the parent's
   * child list via @ref Process::NextSibling (intrusive singly-linked list
   * rooted at @ref Process::FirstChild).
   */
  struct Process {
    /**
     * @brief @ref ProcessID of the @ref Process.
     */
    ProcessID ID;

    /**
     * @brief The @ref Process name.
     */
    char Name[ProcessNameMaxLength];

    /**
     * @brief The @ref Process current @ref ProcessState.
     */
    ProcessState State;

    /**
     * @brief Pointer to the parent @ref Process.
     */
    Process* Parent;

    /**
     * @brief Pointer to the main @ref Thread for the @ref Process.
     */
    Thread* MainThread;

    /**
     * @brief Number of active @ref Thread instances in the @ref Process.
     */
    Size ThreadCount;

    /**
     * @brief Exit code, valid after termination.
     */
    Int32 ExitCode;

    /**
     * @brief Pointer to next @ref Process in the global @ref Process list.
     */
    Process* Next;

    /**
     * @brief Pointer to previous @ref Process in the global @ref Process list.
     */
    Process* Previous;

    /**
     * @brief Pointer to first child @ref Process.
     */
    Process* FirstChild;

    /**
     * @brief Pointer to next sibling @ref Process.
     */
    Process* NextSibling;

    /**
     * @brief Pointer to the @ref Process instance's @ref IAddressSpace.
     */
    IAddressSpace* AddressSpace;

    /**
     * @brief Map of @ref MemoryBlock instances mapped in the @ref Process
     *        address space (@ref IAddressSpace), keyed by @ref Process
     *        address.
     */
    ProcessAddressSpaceMap AddressSpaceMap;

    /**
     * @brief The @ref Process instance's @ref ProcessPermissions.
     */
    ProcessPermissions Permissions;

    /**
     * @brief Number of @ref MemoryBlock instances currently mapped to 
     *        @ref Process heap.
     *
     * Tracks allocations from @ref KernelOperation::Memory_Allocate,
     * demand-paged faults, and @ref KernelOperation::IPC_Receive.
     */
    Size HeapBlockCount = 0;

    /**
     * @brief Total number of @ref MemoryBlock instances mapped to the
     *        @ref Process.
     */
    Size TotalBlockCount = 0;

    /**
     * @brief @ref LinkedList of @ref Thread pointers waiting for the
     *        @ref Process to exit.
     */
    LinkedList<Thread*> WaitQueue;
  };
}
