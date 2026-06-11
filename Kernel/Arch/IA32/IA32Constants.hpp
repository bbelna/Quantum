/**
 * @file Kernel/Arch/IA32/IA32Constants.hpp
 * @brief Declares @ref @QKrnlIA32 constants.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

/**
 * @brief Base process address of the kernel heap.
 */
#define HEAP_BASE 0xC2000000

/**
 * @brief Size of the kernel heap in bytes.
 */
#define HEAP_SIZE_IN_BYTES (512u * MB)

/**
 * @brief Number of guard pages to place before the heap.
 */
#define HEAP_GUARD_PAGE_COUNT_BEFORE 1

/**
 * @brief Number of guard pages to place after the heap.
 */
#define HEAP_GUARD_PAGE_COUNT_AFTER 1

/**
 * @brief Magic value for aligned heap allocations.
 */
#define HEAP_ALIGNED_MAGIC 0xA11A0CEDu

/**
 * @brief Poison byte for allocated heap memory.
 */
#define HEAP_POISON_ALLOCATED 0xAAu

/**
 * @brief Poison byte for freed heap memory.
 */
#define HEAP_POISON_FREED 0x55u

/**
 * @brief Canary value for heap allocations.
 */
#define HEAP_CANARY_VALUE 0xDEADC0DEu

/**
 * @brief Sentinel value for detecting heap overruns.
 */
#define HEAP_ALLOCATED_SENTINEL 0xBAADF00Du

/**
 * @brief Minimum required tail pages to keep free for the heap to satisfy
 *        future allocation requests.
 */
#define HEAP_REQUIRED_TAIL_PAGE_COUNT 2

/**
 * @brief Base process address for stacks.
 */
#define STACK_BASE (HEAP_BASE + HEAP_SIZE_IN_BYTES)

/**
 * @brief Total bytes allocated for stacks.
 */
#define STACK_SIZE_IN_BYTES (64 * MB)

/**
 * @brief Number of guard pages placed below each stack.
 */
#define STACK_GUARD_PAGE_COUNT 1

namespace Quantum::Kernel::Arch::IA32 {
  /**
   * @brief Maximum kernel address supported (4 GiB - 1).
   */
  constexpr UInt32 MaxAddress = 0xFFFFFFFFU;

  /**
   * @brief Maximum kernel address supported (4 GiB - 1) as a `UInt64`.
   */
  constexpr UInt64 MaxAddress64 = 0xFFFFFFFFULL;
}
