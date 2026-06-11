/**
 * @file Kernel/KernelConstants.hpp
 * @brief Declares compile-time build configuration macros for the
 *        @ref @QKrnl.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Definitions.hpp>
#include <Quantum/Core/Logging.hpp>
#include <Quantum/Core/Types.hpp>

#include <KernelTypes.hpp>

/**
 * @brief Size of the @ref @QKrnl::Memory::ObjectPool for
 *        @ref @QKrnl::IPC::IPCPort instances.
 */
#define IPC_PORT_POOL_SIZE 16

/**
 * @brief Size of the @ref @QKrnl::Memory::ObjectPool for
 *        @ref @QKrnl::IPC::IPCMessage instances.
 */
#define IPC_MESSAGE_POOL_SIZE 64

/**
 * @brief Starting point for auto-assigned @ref @QKrnl::IPC::IPCPortID values.
 *
 * This value is chosen to be sufficiently high to avoid collisions with
 * well-known server port IDs, which are typically assigned low numbers.
 */
#define IPC_AUTO_ASSIGN_PORT_ID_START 1000

/**
 * @brief Maximum number of simultaneously active auto-assigned
 *        @ref @QKrnl::IPC::IPCPort instances.
 */
#define IPC_MAX_AUTO_ASSIGN_PORTS 1024

/**
 * @brief Size of a @ref @QKrnl::Threading::Thread
 *        @ref @QKrnl::Memory::Stack in bytes.
 */
#define THREAD_STACK_SIZE 65536

/**
 * @brief Maximum number of @ref @QKrnl::Thread instances to wake.
 */
#define THREAD_DEFERRED_WAKE_LIMIT 64

/**
 * @brief Size of the @ref @QKrnl::Memory::ObjectPool for
 *        @ref @QKrnl::SharedBuffer instances.
 */
#define SHARED_BUFFER_POOL_SIZE 16

/**
 * @brief Maximum number of simultaneously active @ref @QKrnl::Process
 *        instances.
 */
#define PROCESS_MAX_COUNT 1024

/**
 * @brief Number of buckets in the futex wait queue hash table.
 */
#define FUTEX_BUCKET_COUNT 64

/**
 * @brief Maximum length of a @ref @QKrnl::Thread name (including `null`
 *        terminator).
 */
#define THREAD_NAME_MAX_LENGTH 32

/**
 * @brief Stringify helper macro.
 */
#define _STRINGIFY(x) #x

/**
 * @brief Stringify macro that expands its argument before stringifying.
 */
#define STRINGIFY(x) _STRINGIFY(x)

namespace Quantum::Kernel {
  constexpr LogLevel SerialCOMLogLevel = LogLevel::Trace;

  /**
   * @brief Maximum number of @ref Thread instances per @ref Process.
   */
  constexpr Size MaxThreadsPerProcess = 256;

  /**
   * @brief Maximum length of a @ref Process name, including null terminator.
   */
  constexpr Size ProcessNameMaxLength = 64;

  /**
   * @brief Default kernel stack size for @ref Thread instances.
   */
  constexpr Size DefaultKernelThreadStackSize = 16 * KB;

  /**
   * @brief Default user stack size for @ref Thread instances.
   */
  constexpr Size DefaultUserThreadStackSize = 64 * KB;

  /**
   * @brief Maximum number of @ref Thread instances supported.
   */
  constexpr Size MaxThreadCount = 4096;

  /**
   * @brief Size of a @ref MemoryBlock, in bytes.
   */
  constexpr Size MemoryBlockSize = 4096;

  /**
   * @brief Maximum @ref AddressSpaceMap tree depth.
   */
  constexpr Size MaxTreeDepth = 64;

  /**
  * @brief Size of a @ref Thread @ref Stack in bytes.
  */
  constexpr UInt32 ThreadStackSize = THREAD_STACK_SIZE;
}
