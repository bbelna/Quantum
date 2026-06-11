/**
 * @file Kernel/Arch/IA32/Memory/IA32PageConstants.hpp
 * @brief Declares paging constants for @ref @QKrnlIA32::Memory.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Definitions.hpp>

/**
 * @brief Page size for IA32 (4 KB).
 */
#define PAGE_SIZE (4u * KB)

/**
 * @brief Number of entries in an IA-32 page directory.
 */
#define PAGE_DIRECTORY_ENTRY_COUNT 1024

/**
 * @brief Size of an IA-32 page directory entry in bytes.
 */
#define PAGE_DIRECTORY_ENTRY_SIZE 4

/**
 * @brief Size of an IA-32 page table entry in bytes.
 */
#define PAGE_TABLE_ENTRY_SIZE 4

/**
 * @brief Number of entries in an IA-32 page table.
 */
#define PAGE_TABLE_ENTRY_COUNT 1024

/**
 * @brief Recursive page table slot index.
 *
 * Page directory entry 1023 points back to the page directory itself,
 * creating a recursive mapping. This allows the kernel to access any
 * page table at virtual address `0xFFC00000 + (pdIndex * PAGE_SIZE)`
 * and the page directory itself at `0xFFFFF000`.
 */
#define PAGE_TABLE_RECURSIVE_SLOT 1023
