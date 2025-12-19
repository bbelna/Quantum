/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Memory.hpp
 * IA32 paging and memory functions.
 */

#pragma once

#include <Arch/IA32/Interrupts.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 paging and memory functions.
   */
  class Memory {
    public:
      /**
       * State of the physical allocator.
       */
      struct PhysicalAllocatorState {
        /**
         * Total pages managed by the allocator.
         */
        UInt32 TotalPages;

        /**
         * Pages currently marked used.
         */
        UInt32 UsedPages;

        /**
         * Pages currently available.
         */
        UInt32 FreePages;
      };

      /**
       * Base virtual address where the kernel will be mapped in the
       * higher-half. Identity mappings remain available for now to ease the
       * transition.
       */
      static constexpr UInt32 KernelVirtualBase = 0xC0000000;

      /**
       * Page-directory slot reserved for the recursive/self map.
       */
      static constexpr UInt32 RecursiveSlot = 1023;

      /**
       * Initializes paging with identity mappings based on the boot memory map.
       * @param bootInfoPhysicalAddress
       *   Physical address of boot info provided by bootloader.
       */
      static void InitializePaging(UInt32 bootInfoPhysicalAddress);

      /**
       * Allocates a 4 KB physical page (identity mapped).
       * @param zero
       *   Whether to zero the page before returning it.
       * @return
       *   Pointer to the allocated page.
       */
      static void* AllocatePage(bool zero = false);

      /**
       * Frees a 4 KB physical page previously allocated.
       * @param page
       *   Pointer to the physical page to free.
       */
      static void FreePage(void* page);

      /**
       * Maps a virtual page to a physical page with present/RW bits set.
       * Assumes identity mapping for page tables themselves.
       * @param virtualAddress
       *   Virtual address of the page to map.
       * @param physicalAddress
       *   Physical address of the page to map.
       * @param writable
       *   Whether the page should be writable.
       * @param user
       *   Whether the page should be user accessible.
       * @param global
       *   Whether the mapping should be marked global.
       */
      static void MapPage(
        UInt32 virtualAddress,
        UInt32 physicalAddress,
        bool writable = true,
        bool user = false,
        bool global = false
      );

      /**
       * Maps a contiguous virtual range to a contiguous physical range.
       * @param virtualAddress
       *   Base virtual address (page aligned).
       * @param physicalAddress
       *   Base physical address (page aligned).
       * @param lengthBytes
       *   Length of range in bytes.
       * @param writable
       *   Whether the pages should be writable.
       * @param user
       *   Whether the pages should be user accessible.
       * @param global
       *   Whether the mapping should be marked global.
       */
      static void MapRange(
        UInt32 virtualAddress,
        UInt32 physicalAddress,
        UInt32 lengthBytes,
        bool writable = true,
        bool user = false,
        bool global = false
      );

      /**
       * Unmaps a virtual page.
       * Physical pages must be freed separately if desired.
       * @param virtualAddress
       *   Virtual address of the page to unmap.
       */
      static void UnmapPage(UInt32 virtualAddress);

      /**
       * Unmaps a contiguous virtual range.
       * Physical pages must be freed separately if desired.
       * @param virtualAddress
       *   Base virtual address (page aligned).
       * @param lengthBytes
       *   Length of range in bytes.
       */
      static void UnmapRange(UInt32 virtualAddress, UInt32 lengthBytes);

      /**
       * Marks a physical range as used so it will not be handed out by the
       * allocator.
       * @param physicalAddress
       *   Base physical address of the range.
       * @param lengthBytes
       *   Length of the range in bytes.
       */
      static void ReservePhysicalRange(
        UInt32 physicalAddress,
        UInt32 lengthBytes
      );

      /**
       * Releases a previously reserved physical range back to the allocator.
       * @param physicalAddress
       *   Base physical address of the range.
       * @param lengthBytes
       *   Length of the range in bytes.
       */
      static void ReleasePhysicalRange(
        UInt32 physicalAddress,
        UInt32 lengthBytes
      );

      /**
       * Returns the page directory entry value for a given virtual address.
       * @param virtualAddress
       *   Virtual address to query.
       * @return
       *  Page directory entry value for the given virtual address.
       */
      static UInt32 GetPageDirectoryEntry(UInt32 virtualAddress);

      /**
       * Returns the page table entry value for a given virtual address.
       * @param virtualAddress
       *   Virtual address to query.
       * @return
       *   Page table entry value for the given virtual address.
       */
      static UInt32 GetPageTableEntry(UInt32 virtualAddress);

      /**
       * Retrieves the state of the physical allocator.
       * @return
       *   State of the physical allocator.
       */
      static PhysicalAllocatorState GetPhysicalAllocatorState();

      /**
       * Handles a page fault. Currently a stub that logs fault information and
       * returns false to signal an unhandled fault; future implementations can
       * resolve faults (e.g., demand paging).
       * @param context
       *   Interrupt context at the time of the fault.
       * @param faultAddress
       *   Address that triggered the fault (CR2).
       * @param errorCode
       *   Page-fault error code from the CPU.
       * @return
       *   True if the fault was handled; false if it should escalate.
       */
      static bool HandlePageFault(
        const Interrupts::Context& context,
        UInt32 faultAddress,
        UInt32 errorCode
      );
  };
}
