/**
 * @file Kernel/Arch/IA32/Memory/IA32PageTableEntry.hpp
 * @brief Declares @ref @QKrnlIA32::Memory::PageTableEntry.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

#include "IA32PageConstants.hpp"
#include "IA32PageTableFlags.hpp"

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief IA-32 page table entry.
   *
   * Non-PAE paging. 1024 PTEs per table. Each 4-byte entry encodes a
   * present bit, access permissions, caching policy, hardware-maintained
   * accessed/dirty bits, and the 4 KB-aligned physical address of the
   * mapped frame (bits 12-31). Provides both raw `Value` access and a
   * named `bits` bitfield for individual flag manipulation.
   */
  struct [[gnu::packed]] IA32PageTableEntry {
    union {
      /**
       * @brief The raw 32-bit value of the page table entry.
       */
      UInt32 Value;

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
         * @brief Write-through bit.
         */
        UInt32 WriteThrough : 1;

        /**
         * @brief Cache disable bit.
         */
        UInt32 CacheDisable : 1;

        /**
         * @brief Accessed bit.
         */
        UInt32 Accessed : 1;

        /**
         * @brief Dirty bit.
         */
        UInt32 Dirty : 1;

        /**
         * @brief Page attribute table bit.
         */
        UInt32 PAT : 1;

        /**
         * @brief Global page bit.
         */
        UInt32 Global : 1;

        /**
         * @brief Available to operating system.
         */
        UInt32 Available : 3;

        /**
         * @brief Physical address of the 4 KB physical frame (4 KB-aligned).
         */
        UInt32 FrameBase : 20;
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
     * @param present `true` to mark the page as present.
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
     * @param writable `true` to mark the page as writable.
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
     * @brief Returns whether the page is dirty.
     * @return `true` if the dirty bit (bit 6) is set by the CPU on a write.
     */
    constexpr bool IsDirty() const {
      return bits.Dirty != 0;
    }

    /**
     * @brief Sets the dirty bit.
     * @param dirty `true` to set dirty.
     */
    constexpr void SetDirty(bool dirty) {
      bits.Dirty = dirty ? 1 : 0;
    }

    /**
     * @brief Returns whether PAT is set.
     * @return `true` if the PAT bit (bit 7) is set. Combined with PWT and
     *         PCD, selects the memory type from the Page Attribute Table.
     */
    constexpr bool IsPAT() const {
      return bits.PAT != 0;
    }

    /**
     * @brief Sets the PAT bit.
     * @param pat `true` to set PAT.
     */
    constexpr void SetPAT(bool pat) {
      bits.PAT = pat ? 1 : 0;
    }

    /**
     * @brief Returns whether the page is global.
     * @return `true` if the global bit (bit 8) is set, preventing TLB
     *         flush on CR3 reload.
     */
    constexpr bool IsGlobal() const {
      return bits.Global != 0;
    }

    /**
     * @brief Sets the global bit.
     * @param global `true` to set global.
     */
    constexpr void SetGlobal(bool global) {
      bits.Global = global ? 1 : 0;
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
     * @brief Retrieves the page table flags.
     * @return A `PageTableFlag` bitmask reflecting all set hardware bits
     *         (present, writable, user, write-through, cache-disable,
     *         accessed, dirty, PAT, global).
     */
    constexpr IA32PageTableFlags GetFlags() const {
      IA32PageTableFlags flags = IA32PageTableFlags::None;

      if (bits.Present) {
        flags = flags | IA32PageTableFlags::Present;
      }

      if (bits.Writable) {
        flags = flags | IA32PageTableFlags::Write;
      }

      if (bits.User) {
        flags = flags | IA32PageTableFlags::User;
      }

      if (bits.WriteThrough) {
        flags = flags | IA32PageTableFlags::WriteThrough;
      }

      if (bits.CacheDisable) {
        flags = flags | IA32PageTableFlags::CacheDisable;
      }

      if (bits.Accessed) {
        flags = flags | IA32PageTableFlags::Accessed;
      }

      if (bits.Dirty) {
        flags = flags | IA32PageTableFlags::Dirty;
      }

      if (bits.PAT) {
        flags = flags | IA32PageTableFlags::PAT;
      }

      if (bits.Global) {
        flags = flags | IA32PageTableFlags::Global;
      }

      return flags;
    }

    /**
     * @brief Enables the specified page table flags.
     * @param flags The flags to enable.
     */
    constexpr void EnableFlags(IA32PageTableFlags flags) {
      if (Enum::HasAnyFlag(flags, IA32PageTableFlags::Present)) {
        bits.Present = 1;
      }

      if (Enum::HasAnyFlag(flags, IA32PageTableFlags::Write)) {
        bits.Writable = 1;
      }

      if (Enum::HasAnyFlag(flags, IA32PageTableFlags::User)) {
        bits.User = 1;
      }

      if (Enum::HasAnyFlag(flags, IA32PageTableFlags::WriteThrough)) {
        bits.WriteThrough = 1;
      }

      if (Enum::HasAnyFlag(flags, IA32PageTableFlags::CacheDisable)) {
        bits.CacheDisable = 1;
      }

      if (Enum::HasAnyFlag(flags, IA32PageTableFlags::Accessed)) {
        bits.Accessed = 1;
      }

      if (Enum::HasAnyFlag(flags, IA32PageTableFlags::Dirty)) {
        bits.Dirty = 1;
      }

      if (Enum::HasAnyFlag(flags, IA32PageTableFlags::PAT)) {
        bits.PAT = 1;
      }

      if (Enum::HasAnyFlag(flags, IA32PageTableFlags::Global)) {
        bits.Global = 1;
      }
    }

    /**
     * @brief Sets the physical frame base address.
     * @param physicalBase 4 KB-aligned physical address of the frame.
     */
    constexpr void SetFrameBase(UIntPtr physicalBase) {
      bits.FrameBase = static_cast<UInt32>(physicalBase >> 12);
    }

    /**
     * @brief Returns the physical frame base address.
     * @return The 4 KB-aligned physical address of the mapped frame.
     */
    constexpr UIntPtr GetFrameBase() const {
      return static_cast<UIntPtr>(bits.FrameBase) << 12;
    }
  };

  static_assert(sizeof(IA32PageTableEntry) == PAGE_TABLE_ENTRY_SIZE);
}
