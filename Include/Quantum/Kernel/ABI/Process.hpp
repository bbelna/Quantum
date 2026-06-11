/**
 * @file Include/Quantum/Kernel/ABI/Process.hpp
 * @brief Declaration of the kernel process ABI.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Address.hpp>
#include <Quantum/Kernel/Arch.hpp>
#include <Quantum/Core/Logging.hpp>
#include <Quantum/Kernel/Memory/MemoryTypes.hpp>
#include <Quantum/Threading/Process.hpp>
#include <Quantum/Core/Types.hpp>

#include "ABI.hpp"
#include "../Concurrency.hpp"

/**
 * @brief ABI functions for process management.
 */
namespace Quantum::Kernel::ABI::Process {
  /**
   * @brief Information about a running process.
   */
  struct ProcessInfo {
    /**
     * @brief The process ID.
     */
    Concurrency::ProcessID ID;

    /**
     * @brief The process name.
     */
    char Name[64];

    /**
     * @brief The process state (maps to kernel `ProcessState` enum).
     */
    UInt8 State;

    /**
     * @brief Number of active threads in this process.
     */
    Size ThreadCount;

    /**
     * @brief Number of heap pages mapped to this process.
     */
    Size HeapPageCount;

    /**
     * @brief Total number of physical pages mapped to this process.
     */
    Size TotalPageCount;
  };

  /**
   * @brief Gets a list of all running processes.
   * @param buffer Pointer to an array of `ProcessInfo` structures to fill.
   * @param maxCount Maximum number of entries to write.
   * @return The number of entries written.
   */
  inline UInt32 GetList(ProcessInfo* buffer, UInt32 maxCount) {
    return Invoke(
      KernelOperation::Process_GetList,
      reinterpret_cast<UInt32>(buffer),
      maxCount
    );
  }

  /**
   * @brief Describes a single loadable segment within a spawn binary buffer.
   *        Used to communicate per-segment memory permissions from the ELF
   *        loader to the kernel.
   */
  struct SpawnSegment {
    /**
     * @brief Byte offset of this segment from `SpawnParameters::SourceBase`.
     */
    Size OffsetInBytes;

    /**
     * @brief Size of this segment in bytes.
     */
    Size SizeInBytes;

    /**
     * @brief Permission flags for this segment.
     *        Bit 0 = Read, Bit 1 = Write, Bit 2 = Execute.
     */
    UInt32 Permissions;
  };

  /**
   * @brief Parameters for spawning a new process with binary mapping.
   */
  struct SpawnParameters {
    /**
     * @brief Source virtual address of the binary data in the caller's
     *        address space.
     */
    UIntPtr SourceBase;

    /**
     * @brief Size of the binary image in bytes.
     */
    Size SizeInBytes;

    /**
     * @brief Target virtual address in the child's address space where the
     *        binary should be mapped. When zero, the kernel uses the
     *        page-aligned base of `entryPoint` instead.
     */
    UIntPtr TargetBase;

    /**
     * @brief Number of segment descriptors in the `Segments` array. When zero,
     *        all pages are mapped with read, write, and user permissions.
     */
    Size SegmentCount;

    /**
     * @brief Pointer to an array of `SpawnSegment` descriptors in the
     *        caller's address space, or `nullptr` if `SegmentCount` is zero.
     */
    const SpawnSegment* Segments;

    /**
     * @brief Number of command-line arguments.
     */
    Size ArgumentCount = 0;

    /**
     * @brief Pointer to packed null-terminated argument strings in the
     *        caller's address space. The strings are concatenated with a
     *        `\0` separator between each. For example, two arguments
     *        `"ls"` and `"-la"` are stored as `"ls\0-la\0"`.
     */
    const char* ArgumentData = nullptr;

    /**
     * @brief Total size in bytes of the packed argument data, including
     *        all null terminators.
     */
    Size ArgumentDataSize = 0;

    /**
     * @brief Number of inherited stream buffer IDs (0 to 3). When non-zero,
     *        the kernel writes the buffer IDs into the child's process
     *        stream table at a fixed virtual address.
     */
    UInt8 StreamCount = 0;

    /**
     * @brief Inherited stream buffer IDs. Index 0 = stdin, 1 = stdout,
     *        2 = stderr. A value of `0` means no stream for that index.
     */
    Memory::SharedBufferID StreamBufferIDs[3] = {};
  };

  /**
   * @brief Exits the current process with the given status code.
   * @param statusCode The exit status code.
   */
  inline void Exit(UInt32 statusCode) {
    Invoke(
      KernelOperation::Process_Exit,
      statusCode,
      0,
      0
    );
  }

  /**
   * @brief Spawns a new process with the given parameters.
   * @param name The process' name.
   * @param entryPoint Virtual address of the process' entry point.
   * @param parameters Optional spawn parameters for binary mapping, or
   *                   `nullptr` for no mapping.
   * @return The new process' ID on success, or -1 on failure.
   */
  inline Concurrency::ProcessID Spawn(
    const char* name,
    UIntPtr entryPoint,
    const SpawnParameters* parameters = nullptr
  ) {
    return Invoke(
      KernelOperation::Process_Spawn,
      reinterpret_cast<UInt32>(name),
      entryPoint,
      reinterpret_cast<UInt32>(parameters)
    );
  }

  /**
   * @brief Requests a specific permission for the current process.
   * @param permission The permission to request.
   * @return `true` if the permission was granted; `false` otherwise.
   */
  inline bool RequestPermission(Threading::ProcessPermissions permission) {
    UInt32 result = Invoke(
      KernelOperation::Process_RequestPermission,
      static_cast<UInt32>(permission),
      0,
      0
    );

    return result != 0;
  }

  /**
   * @brief Waits for a process to exit.
   * @param pid The process ID to wait for.
   * @return The exit code of the process, or -1 on failure.
   */
  inline Int32 Wait(Concurrency::ProcessID pid) {
    return static_cast<Int32>(Invoke(
      KernelOperation::Process_Wait,
      static_cast<UInt32>(pid),
      0,
      0
    ));
  }

  /**
   * @brief Gets the process ID of the current process.
   * @return The current process ID, or -1 on failure.
   */
  inline Concurrency::ProcessID GetID() {
    return Invoke(
      KernelOperation::Process_GetID,
      0,
      0,
      0
    );
  }
}
