/**
 * @file Kernel/Arch/IA32/Bootstrap/IA32BootstrapStage1.cpp
 * @brief Implements @ref @QKrnlIA32::Bootstrap::IA32BootstrapStage1.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Arch/IA32/Drivers/CPU/IA32CPUDriver.hpp>
#include <Arch/IA32/IA32LinkerSymbols.hpp>
#include <Arch/IA32/Memory/IA32AddressTranslator.hpp>
#include <Arch/IA32/Memory/IA32PageDirectoryFlag.hpp>
#include <Arch/IA32/Memory/IA32MemoryMapper.hpp>
#include <Arch/IA32/Memory/IA32MemoryTypes.hpp>

#include "IA32BootstrapStage1.hpp"
#include "IA32BootstrapStage2.hpp"

/**
 * @brief Number of kernel page tables.
 */
#define KERNEL_PAGE_TABLE_COUNT 8

namespace Quantum::Kernel::Arch::IA32::Bootstrap {
  namespace {
    /**
     * @brief The page tables for initial paging setup.
     */
    [[gnu::section(".start.data")]]
    alignas(PAGE_SIZE)
    UInt32 InitialPageDirectory[PAGE_DIRECTORY_ENTRY_COUNT];

    /**
     * @brief Kernel higher-half page tables.
     */
    [[gnu::section(".start.data")]]
    alignas(PAGE_SIZE)
    UInt32 KernelTables[KERNEL_PAGE_TABLE_COUNT][PAGE_TABLE_ENTRY_COUNT];

    /**
     * @brief Retrieves the current stack pointer (ESP).
     * @return The current stack pointer.
     */
    [[gnu::section(".text.start")]]
    UInt32 GetESP() {
      UInt32 esp;

      asm volatile("mov %%esp, %0" : "=r"(esp));

      return esp;
    }

    /**
     * @brief Builds identity and higher-half mappings needed to turn on paging.
     * @return Kernel address of the page directory used for initial paging.
     */
    [[gnu::section(".text.start")]]
    UInt32 InitializePaging() {
      // clear the page directory
      for (
        UInt32 pageDirectoryIndex = 0;
        pageDirectoryIndex < PAGE_TABLE_ENTRY_COUNT;
        ++pageDirectoryIndex
      ) {
        InitialPageDirectory[pageDirectoryIndex] = 0;
      }

      // enable PSE (Page Size Extensions) for 4 MB page support
      UInt32 cr4;

      asm volatile("mov %%cr4, %0" : "=r"(cr4));

      cr4 |= (1u << 4); // CR4.PSE

      asm volatile("mov %0, %%cr4" : : "r"(cr4) : "memory");

      // identity map low memory using 4 MB PSE pages (up to kernel
      // higher-half base at 0xC0000000, i.e. entries 0-767)
      constexpr UInt32 identityMapEntries = 0xC0000000u >> 22; // 768

      for (UInt32 i = 0; i < identityMapEntries; ++i) {
        InitialPageDirectory[i]
          = (i << 22)
          | IA32PageDirectoryFlag::Present
          | IA32PageDirectoryFlag::Write
          | IA32PageDirectoryFlag::PageSize;
      }

      // map kernel higher-half: map the loaded higher-half image
      UInt32 kernelImageBytes = KERNEL_PHYSICAL_END
                              - KERNEL_HIGHER_HALF_PHYSICAL_BASE;
      UInt32 nextKernelTable = 0;

      // map each page of the kernel image
      for (
        UInt32 offset = 0;
        offset < kernelImageBytes;
        offset += PAGE_SIZE
      ) {
        UInt32 physicalAddress = KERNEL_HIGHER_HALF_PHYSICAL_BASE + offset;
        UInt32 virtualAddress = KERNEL_HIGHER_HALF_VIRTUAL_BASE + offset;
        UInt32 pageDirectoryIndex = (virtualAddress >> 22) & 0x3FF;
        UInt32 pageTableIndex = (virtualAddress >> 12) & 0x3FF;

        // create page table if not present
        if (
          InitialPageDirectory[pageDirectoryIndex] == 0 &&
          nextKernelTable < KERNEL_PAGE_TABLE_COUNT
         ) {
          UInt32* table = KernelTables[nextKernelTable++];

          // clear the table
          for (
            UInt32 kernelTableIndex = 0;
            kernelTableIndex < PAGE_TABLE_ENTRY_COUNT;
            ++kernelTableIndex
          ) {
            table[kernelTableIndex] = 0;
          }

          // set page directory entry
          InitialPageDirectory[pageDirectoryIndex]
            = reinterpret_cast<UInt32>(table)
            | IA32PageDirectoryFlag::Present
            | IA32PageDirectoryFlag::Write;
        }

        // set page table entry
        UInt32* table = reinterpret_cast<UInt32*>(
          InitialPageDirectory[pageDirectoryIndex] & ~0xFFFu
        );

        table[pageTableIndex]
          = physicalAddress
          | IA32PageDirectoryFlag::Present
          | IA32PageDirectoryFlag::Write;
      }

      // install recursive mapping
      InitialPageDirectory[PAGE_TABLE_RECURSIVE_SLOT]
        = reinterpret_cast<UInt32>(InitialPageDirectory)
        | IA32PageDirectoryFlag::Present
        | IA32PageDirectoryFlag::Write;

      // load page directory
      UInt32 pageDirectoryPhysicalAddress = reinterpret_cast<UInt32>(
        InitialPageDirectory
      );

      asm volatile(
        "mov %0, %%cr3" :: "r"(pageDirectoryPhysicalAddress) : "memory"
      );

      UInt32 cr0;

      asm volatile("mov %%cr0, %0" : "=r"(cr0));

      cr0 |= 0x80000000;

      asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");

      return pageDirectoryPhysicalAddress;
    }

    /**
     * @brief
     *   Jumps to stage 2 in the higher-half address space.
     * @param bootInfoPhysicalAddress
     *   Physical address of the boot info structure.
     */
    [[gnu::section(".text.start")]]
    void JumpToStage2(UInt32 bootInfoPhysicalAddress) {
      UInt32 start = reinterpret_cast<UInt32>(&IA32BootstrapStage2);
      UInt32 esp = GetESP(); // keep using the low stack

      // jump to higher-half start with adjusted stack
      asm volatile(
        "mov %0, %%esp\n"
        "push %1\n"
        "call *%2\n"
        :
        : "r"(esp), "r"(bootInfoPhysicalAddress), "r"(start)
        : "memory"
      );
    }

    /**
     * @brief Zeroes the kernel BSS segment using kernel addresses.
     *
     * Must be called before paging is enabled, while executing from
     * identity-mapped `.text.start` code.
     */
    [[gnu::section(".text.start")]]
    void ZeroBSS() {
      UInt32* start = reinterpret_cast<UInt32*>(&__phys_bss_start);
      UInt32* end = reinterpret_cast<UInt32*>(&__phys_bss_end);

      for (UInt32* p = start; p < end; ++p) *p = 0;
    }
  }

  extern "C"
  [[noreturn]]
  [[gnu::section(".text.start")]]
  void IA32BootstrapStage1(UInt32 bootInfoPhysicalAddress) {
    ZeroBSS();
    InitializePaging();
    JumpToStage2(bootInfoPhysicalAddress);

    // if we ever return from stage 2 (we shouldn't),
    // disable interrupts ...
    asm volatile("cli");

    // .. and halt forever
    for (;;) {
      asm volatile("hlt");
    }
  }
}
