/**
 * @file Kernel/Arch/IA32/Memory/IA32PageDirectoryEntry.hpp
 * @brief Declares @ref @QKrnlIA32::Memory::PageDirectoryEntry.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "IA32PageConstants.hpp"

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief IA-32 page directory entry.
   *
   * Non-PAE paging. 1024 PDEs per directory. Each 4-byte entry
   * encodes a present bit, access permissions, caching policy, and the
   * 4 KB-aligned physical address of a page table (bits 12-31).
   * Provides both raw `Value` access and a named `bits` bitfield for
   * individual flag manipulation.
   */
  struct IA32PageDirectoryEntry {
    union {
      /**
       * @brief The raw 32-bit value of the page directory entry.
       */
      UInt32 Value;

      /**
       * @brief The individual bit fields of the page directory entry.
       */
      struct {
        /**
         * @brief Present bit (1 = present, 0 = not present).
         */
        UInt32 Present : 1;

        /**
         * @brief Read/Write bit (1 = writable, 0 = read-only).
         */
        UInt32 Writable : 1;

        /**
         * @brief User/Supervisor bit (1 = user-mode, 0 = supervisor-mode).
         */
        UInt32 User : 1;

        /**
         * @brief Page-level write-through.
         */
        UInt32 WriteThrough : 1;

        /**
         * @brief Page-level cache disable.
         */
        UInt32 CacheDisable : 1;

        /**
         * @brief Accessed bit.
         */
        UInt32 Accessed : 1;

        /**
         * @brief Reserved; must be zero for 4 KB pages.
         */
        UInt32 Reserved0 : 1;

        /**
         * @brief Page size (0 = 4 KB pages).
         */
        UInt32 PageSize : 1;

        /**
         * @brief Ignored (or available to operating system).
         */
        UInt32 Ignored0 : 1;

        /**
         * @brief Available to operating system.
         */
        UInt32 Available : 3;

        /**
         * @brief Physical address of the page table (4 KB-aligned).
         */
        UInt32 TableBase : 20;
      } __attribute__((packed)) bits;
    };

    /**
     * @brief Clears the entry to zero.
     */
    constexpr void Clear() { Value = 0; }

    /**
     * @brief Returns whether the page is present in memory.
     * @return `true` if the present bit (bit 0) is set.
     */
    constexpr bool IsPresent() const {
      return bits.Present != 0;
    }

    /**
     * @brief Sets the present bit.
     * @param present `true` to mark the page as present, false otherwise.
     */
    constexpr void SetPresent(bool present) {
      bits.Present = present ? 1 : 0;
    }

    /**
     * @brief Returns whether the page is writable.
     * @return `true` if the read/write bit (bit 1) is set.
     */
    constexpr bool IsWritable() const {
      return bits.Writable != 0;
    }

    /**
     * @brief Sets the writable bit.
     * @param writable `true` to mark the page as writable, false otherwise.
     */
    constexpr void SetWritable(bool writable) {
      bits.Writable = writable ? 1 : 0;
    }

    /**
     * @brief Returns whether the page is user-accessible.
     * @return `true` if the user/supervisor bit (bit 2) is set.
     */
    constexpr bool IsUser() const {
      return bits.User != 0;
    }

    /**
     * @brief Sets the user/supervisor bit.
     * @param user `true` to mark as user-accessible.
     */
    constexpr void SetUser(bool user) {
      bits.User = user ? 1 : 0;
    }

    /**
     * @brief Returns whether write-through is enabled.
     * @return `true` if the write-through bit (bit 3) is set.
     */
    constexpr bool IsWriteThrough() const {
      return bits.WriteThrough != 0;
    }

    /**
     * @brief Sets write-through.
     * @param writeThrough `true` to enable write-through.
     */
    constexpr void SetWriteThrough(bool writeThrough) {
      bits.WriteThrough = writeThrough ? 1 : 0;
    }

    /**
     * @brief Returns whether cache is disabled.
     * @return `true` if the cache-disable bit (bit 4) is set.
     */
    constexpr bool IsCacheDisabled() const {
      return bits.CacheDisable != 0;
    }

    /**
     * @brief Sets cache disable.
     * @param cacheDisable `true` to disable caching.
     */
    constexpr void SetCacheDisabled(bool cacheDisable) {
      bits.CacheDisable = cacheDisable ? 1 : 0;
    }

    /**
     * @brief Returns whether the entry has been accessed.
     * @return `true` if the accessed bit (bit 5) is set by the CPU.
     */
    constexpr bool IsAccessed() const {
      return bits.Accessed != 0;
    }

    /**
     * @brief Sets the accessed bit.
     * @param accessed `true` to set accessed.
     */
    constexpr void SetAccessed(bool accessed) {
      bits.Accessed = accessed ? 1 : 0;
    }

    /**
     * @brief Returns whether page size is 4 MB (PS=1).
     * @return `true` if the page-size bit (bit 7) is set, indicating 4 MB
     *         pages instead of a page table reference.
     */
    constexpr bool IsLargePage() const {
      return bits.PageSize != 0;
    }

    /**
     * @brief Sets the page size bit (0 = 4 KB, 1 = 4 MB).
     * @param largePage `true` for 4 MB pages.
     */
    constexpr void SetLargePage(bool largePage) {
      bits.PageSize = largePage ? 1 : 0;
    }

    /**
     * @brief Returns the available bits field.
     * @return The 3-bit OS-defined field (bits 9-11).
     */
    constexpr UInt32 GetAvailable() const {
      return bits.Available;
    }

    /**
     * @brief Sets the available bits field (3 bits).
     * @param available Value to store (low 3 bits used).
     */
    constexpr void SetAvailable(UInt32 available) {
      bits.Available = available & 0x7u;
    }

    /**
     * @brief Sets the page table base physical address.
     * @param physicalBase 4 KB-aligned physical address of a page table.
     */
    constexpr void SetTableBase(UIntPtr physicalBase) {
      bits.TableBase = static_cast<UInt32>(physicalBase >> 12);
    }

    /**
     * @brief Returns the page table base physical address.
     * @return The 4 KB-aligned physical address of the referenced page table.
     */
    constexpr UIntPtr GetTableBase() const {
      return static_cast<UIntPtr>(bits.TableBase) << 12;
    }
  } __attribute__((packed));

  static_assert(sizeof(IA32PageDirectoryEntry) == PAGE_DIRECTORY_ENTRY_SIZE);
}
