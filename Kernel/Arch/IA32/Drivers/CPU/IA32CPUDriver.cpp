/**
 * @file Kernel/Arch/IA32/Drivers/CPU/IA32CPUDriver.cpp
 * @brief Implements @ref @QKrnlIA32::Drivers::CPU::IA32CPUDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Arch/IA32/IA32LinkerSymbols.hpp>
#include <Arch/IA32/Memory/IA32PageDirectory.hpp>
#include <Arch/IA32/Memory/IA32PageTable.hpp>
#include <Arch/IA32/Memory/IA32MemoryTypes.hpp>
#include <KernelLog.hpp>

#include "IA32CPUDriver.hpp"

namespace Quantum::Kernel::Arch::IA32::Drivers::CPU {
  IA32CPUDriver::IA32CPUDriver() {
    _probe();
  }

  UInt32 IA32CPUDriver::Invoke(UInt32 operation, void* payload) {
    switch (static_cast<CPUDriverOperation>(operation)) {
      case CPUDriverOperation::GetHypervisorInfo: {
        if (!payload) return 0;

        CPUHypervisorInfo* info
          = static_cast<CPUHypervisorInfo*>(payload);

        *info = _hypervisorInfo;

        return _hypervisorInfo.Present ? 1 : 0;
      }

      case CPUDriverOperation::GetFPUInfo: {
        if (!payload) return 0;

        CPUFPUInfo* info = static_cast<CPUFPUInfo*>(payload);

        *info = _fpuInfo;

        return _fpuInfo.Present ? 1 : 0;
      }

      default: {
        return 0;
      }
    }
  }

  void IA32CPUDriver::Halt() {
    asm volatile("hlt");
  }

  void IA32CPUDriver::HaltForever() {
    DisableInterrupts();

    for (;;) Halt();
  }

  void IA32CPUDriver::Pause() {
    asm volatile("pause");
  }

  void IA32CPUDriver::Yield() {
    asm volatile("int %0" :: "i"(49) : "memory");
  }

  void IA32CPUDriver::DisableInterrupts() {
    asm volatile("cli" ::: "memory");
  }

  void IA32CPUDriver::EnableInterrupts() {
    asm volatile("sti" ::: "memory");
  }

  UInt32 IA32CPUDriver::SaveAndDisableInterrupts() {
    UInt32 flags;

    asm volatile(
      "pushfl\n"
      "pop %0\n"
      "cli"
      : "=r"(flags)
      :
      : "memory"
    );

    return flags;
  }

  void IA32CPUDriver::RestoreInterrupts(UInt32 flags) {
    if (flags & 0x200) {
      asm volatile("sti" ::: "memory");
    }
  }

  UInt8 IA32CPUDriver::In8(UInt16 port) {
    UInt8 value;

    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));

    return value;
  }

  UInt16 IA32CPUDriver::In16(UInt16 port) {
    UInt16 value;

    asm volatile("inw %1, %0" : "=a"(value) : "Nd"(port));

    return value;
  }

  UInt32 IA32CPUDriver::In32(UInt16 port) {
    UInt32 value;

    asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));

    return value;
  }

  void IA32CPUDriver::Out8(UInt16 port, UInt8 value) {
    asm volatile("outb %0, %1" :: "a"(value), "Nd"(port));
  }

  void IA32CPUDriver::Out16(UInt16 port, UInt16 value) {
    asm volatile("outw %0, %1" :: "a"(value), "Nd"(port));
  }

  void IA32CPUDriver::Out32(UInt16 port, UInt32 value) {
    asm volatile("outl %0, %1" :: "a"(value), "Nd"(port));
  }

  UInt32 IA32CPUDriver::Exchange32(
    volatile UInt32* pointer,
    UInt32 value
  ) {
    asm volatile(
      "xchgl %0, %1"
      : "+r"(value), "+m"(*pointer)
      :
      : "memory"
    );

    return value;
  }

  bool IA32CPUDriver::CompareExchange32(
    volatile UInt32* pointer,
    UInt32& expected,
    UInt32 desired
  ) {
    UInt32 previous;

    asm volatile(
      "lock\n"
      "cmpxchgl %3, %1"
      : "=a"(previous), "+m"(*pointer)
      : "a"(expected), "r"(desired)
      : "cc", "memory"
    );

    bool swapped = previous == expected;

    expected = previous;

    return swapped;
  }

  bool IA32CPUDriver::CompareExchange64(
    volatile UInt64* pointer,
    UInt64& expected,
    UInt64 desired
  ) {
    UInt32 expectedLow = (UInt32)(expected & 0xFFFFFFFFull);
    UInt32 expectedHigh = (UInt32)(expected >> 32);
    UInt32 desiredLow = (UInt32)(desired & 0xFFFFFFFFull);
    UInt32 desiredHigh = (UInt32)(desired >> 32);

    UInt8 success;

    asm volatile(
      "lock\ncmpxchg8b %1\n"
      "sete %0"
      : "=q"(success), "+m"(*pointer),
        "+a"(expectedLow), "+d"(expectedHigh)
      : "b"(desiredLow), "c"(desiredHigh)
      : "cc", "memory"
    );

    expected = ((UInt64)expectedHigh << 32) | expectedLow;

    return success != 0;
  }

  UInt32 IA32CPUDriver::FetchAdd32(
    volatile UInt32* pointer,
    UInt32 delta
  ) {
    asm volatile(
      "lock\nxaddl %0, %1"
      : "+r"(delta), "+m"(*pointer)
      :
      : "cc", "memory"
    );

    return delta;
  }

  void IA32CPUDriver::CompilerBarrier() {
    asm volatile("" ::: "memory");
  }

  void IA32CPUDriver::LockedBarrier() {
    UInt32 dummy = 0;

    asm volatile(
      "lock\naddl $0, %0" : "+m"(dummy) :: "cc", "memory"
    );
  }

  void IA32CPUDriver::LoadPageDirectory(UIntPtr pagePhysicalAddress) {
    asm volatile(
      "mov %0, %%cr3" :: "r"(pagePhysicalAddress) : "memory"
    );
  }

  void IA32CPUDriver::EnablePaging() {
    UInt32 cr0;

    asm volatile("mov %%cr0, %0" : "=r"(cr0));

    cr0 |= 0x80000000;

    asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
  }

  void IA32CPUDriver::EnableWriteProtect() {
    KLOG_DEBUG("Enabling write protection");

    UInt32 cr0;

    asm volatile("mov %%cr0, %0" : "=r"(cr0));

    cr0 |= (1u << 16);

    asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
  }

  void IA32CPUDriver::InvalidatePage(UIntPtr pageVirtualAddress) {
    asm volatile("invlpg (%0)" :: "r"(pageVirtualAddress) : "memory");
  }

  UInt64 IA32CPUDriver::ReadMSR(UInt32 msr) {
    UInt32 low, high;

    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));

    return (static_cast<UInt64>(high) << 32) | low;
  }

  void IA32CPUDriver::WriteMSR(UInt32 msr, UInt64 value) {
    UInt32 low = static_cast<UInt32>(value);
    UInt32 high = static_cast<UInt32>(value >> 32);

    asm volatile("wrmsr" :: "c"(msr), "a"(low), "d"(high));
  }

  const char* IA32CPUDriver::GetArchName() {
    return "IA32";
  }

  bool IA32CPUDriver::HasCPUID() {
    return _detectHasCPUID();
  }

  UInt8 IA32CPUDriver::DetectCPUGeneration() {
    return _detectGeneration();
  }

  bool IA32CPUDriver::HasFPU() {
    return _detectHasFPU();
  }

  void IA32CPUDriver::CPUID(
    UInt32 leaf,
    UInt32& eax,
    UInt32& ebx,
    UInt32& ecx,
    UInt32& edx
  ) {
    _executeCPUID(leaf, eax, ebx, ecx, edx);
  }

  void IA32CPUDriver::InitializePAT() {
    _initializePAT();
  }

  bool IA32CPUDriver::IsPATSupported() {
    return _patSupported;
  }

  UIntPtr IA32CPUDriver::VirtualToPhysical(UIntPtr virtualAddress) {
    if (virtualAddress >= KERNEL_VIRTUAL_BASE) {
      UInt32 offset
        = static_cast<UInt32>(virtualAddress - KERNEL_VIRTUAL_BASE);

      return KERNEL_PHYSICAL_BASE + offset;
    }

    if (_kernelPageDirectoryLoaded) {
      UInt32 cr3;

      asm volatile("mov %%cr3, %0" : "=r"(cr3));

      if (cr3 != 0) {
        auto* pageDirectory = reinterpret_cast<IA32PageDirectory*>(cr3);
        UInt32 pageDirectoryIndex
          = IA32PageDirectory::Index(virtualAddress);
        IA32PageDirectoryEntry& pageDirectoryEntry
          = pageDirectory->Entries[pageDirectoryIndex];

        if (pageDirectoryEntry.IsPresent()) {
          auto* pageTable = reinterpret_cast<IA32PageTable*>(
            pageDirectoryEntry.GetTableBase()
          );
          UInt32 pageTableEntryIndex
            = IA32PageTable::Index(virtualAddress);
          IA32PageTableEntry& pageTableEntry
            = pageTable->Entries[pageTableEntryIndex];

          if (pageTableEntry.IsPresent()) {
            return pageTableEntry.GetFrameBase()
              | (virtualAddress & 0xFFF);
          }
        }
      }
    }

    return virtualAddress;
  }

  UIntPtr IA32CPUDriver::PhysicalToVirtual(UIntPtr physicalAddress) {
    if (physicalAddress >= KERNEL_PHYSICAL_BASE) {
      UInt32 offset
        = static_cast<UInt32>(physicalAddress - KERNEL_PHYSICAL_BASE);

      return KERNEL_VIRTUAL_BASE + offset;
    }

    return physicalAddress;
  }

  void IA32CPUDriver::NotifyKernelPageDirectoryLoaded() {
    _kernelPageDirectoryLoaded = true;

    KLOG_DEBUG("Page table walk enabled");
  }

  void IA32CPUDriver::_storeRegister(UInt32 reg, char* destination) {
    destination[0] = static_cast<char>(reg & 0xFF);
    destination[1] = static_cast<char>((reg >> 8) & 0xFF);
    destination[2] = static_cast<char>((reg >> 16) & 0xFF);
    destination[3] = static_cast<char>((reg >> 24) & 0xFF);
  }

  void IA32CPUDriver::_probe() {
    UInt32 eax, ebx, ecx, edx;

    if (!_detectHasCPUID()) {
      UInt8 generation = _detectGeneration();

      CString::Format(
        _device.DisplayName,
        DeviceDisplayNameMaxLength,
        "i%u86 Processor",
        generation
      );

      KLOG_TRACE("CPU DisplayName=\"%s\"", _device.DisplayName);

      _fpuInfo.Present = _detectHasFPU();

      if (_fpuInfo.Present) {
        KLOG_TRACE("FPU detected (external)");
      } else {
        KLOG_TRACE("No FPU detected");
      }

      return;
    }

    _executeCPUID(0, eax, ebx, ecx, edx);

    UInt32 maxStandardLeaf = eax;

    char vendor[13] = {};

    _storeRegister(ebx, vendor);
    _storeRegister(edx, vendor + 4);
    _storeRegister(ecx, vendor + 8);

    vendor[12] = '\0';

    _vendor = IA32CPUVendorFromString(vendor);

    ToDeviceName(_vendor, 0, _device.Name, DeviceNameMaxLength);

    UInt32 family = 0;
    UInt32 model = 0;
    UInt32 stepping = 0;
    UInt32 leaf1Edx = 0;

    if (maxStandardLeaf >= 1) {
      _executeCPUID(1, eax, ebx, ecx, edx);

      leaf1Edx = edx;
      stepping = eax & 0xF;
      model = (eax >> 4) & 0xF;
      family = (eax >> 8) & 0xF;

      UInt32 extendedModel = (eax >> 16) & 0xF;
      UInt32 extendedFamily = (eax >> 20) & 0xFF;

      if (family == 0x6 || family == 0xF) {
        model = model + (extendedModel << 4);
      }

      if (family == 0xF) family = family + extendedFamily;

      _hypervisorInfo.Present = (ecx >> 31) & 1;
    }

    _executeCPUID(0x80000000, eax, ebx, ecx, edx);

    UInt32 maxExtendedLeaf = eax;

    if (maxExtendedLeaf >= 0x80000004) {
      char brand[49] = {};

      for (UInt32 leaf = 0x80000002; leaf <= 0x80000004; ++leaf) {
        _executeCPUID(leaf, eax, ebx, ecx, edx);

        UInt32 offset = (leaf - 0x80000002) * 16;

        _storeRegister(eax, brand + offset);
        _storeRegister(ebx, brand + offset + 4);
        _storeRegister(ecx, brand + offset + 8);
        _storeRegister(edx, brand + offset + 12);
      }

      brand[48] = '\0';

      const char* trimmed = brand;

      while (*trimmed == ' ') trimmed++;

      ToDeviceDisplayName(
        _vendor,
        trimmed,
        _device.DisplayName,
        DeviceDisplayNameMaxLength
      );
    } else {
      ToDeviceDisplayName(
        _vendor,
        family,
        model,
        _device.DisplayName,
        DeviceDisplayNameMaxLength
      );
    }

    KLOG_TRACE(
      "CPU: DisplayName=\"%s\" Family=%u Model=%u Stepping=%u",
      _device.DisplayName,
      family,
      model,
      stepping
    );

    _fpuInfo.Present = _detectHasFPU();

    if (_fpuInfo.Present) {
      if (family >= 5) {
        _fpuInfo.Integrated = true;
      } else if (maxStandardLeaf >= 1) {
        _fpuInfo.Integrated = (leaf1Edx & 1) != 0;
      }

      KLOG_TRACE(
        "FPU: Integrated=%s",
        _fpuInfo.Integrated ? "true" : "false"
      );
    }

    if (_hypervisorInfo.Present) {
      _executeCPUID(0x40000000, eax, ebx, ecx, edx);

      _storeRegister(ebx, _hypervisorInfo.Vendor);
      _storeRegister(ecx, _hypervisorInfo.Vendor + 4);
      _storeRegister(edx, _hypervisorInfo.Vendor + 8);

      _hypervisorInfo.Vendor[12] = '\0';

      KLOG_TRACE("CPUHypervisorInfo.Vendor=\"%s\"", _hypervisorInfo.Vendor);
      KLOG_INFO("Hypervisor detected");
    }
  }

  bool IA32CPUDriver::_detectHasCPUID() {
    UInt32 original, flipped;

    asm volatile(
      "pushfl\n"
      "popl %0\n"
      "movl %0, %1\n"
      "xorl $0x200000, %1\n"
      "pushl %1\n"
      "popfl\n"
      "pushfl\n"
      "popl %1\n"
      "pushl %0\n"
      "popfl"
      : "=&r"(original), "=&r"(flipped)
    );

    return (original ^ flipped) & 0x200000;
  }

  UInt8 IA32CPUDriver::_detectGeneration() {
    UInt32 original, flipped;

    asm volatile(
      "pushfl\n"
      "popl %0\n"
      "movl %0, %1\n"
      "xorl $0x40000, %1\n"
      "pushl %1\n"
      "popfl\n"
      "pushfl\n"
      "popl %1\n"
      "pushl %0\n"
      "popfl"
      : "=&r"(original), "=&r"(flipped)
    );

    return (original ^ flipped) & 0x40000
      ? 4
      : 3;
  }

  bool IA32CPUDriver::_detectHasFPU() {
    UInt16 statusWord = 0x5A5A;

    asm volatile(
      "fninit\n"
      "fnstsw %0"
      : "=m"(statusWord)
    );

    return statusWord == 0;
  }

  void IA32CPUDriver::_executeCPUID(
    UInt32 leaf,
    UInt32& eax,
    UInt32& ebx,
    UInt32& ecx,
    UInt32& edx
  ) {
    asm volatile(
      "cpuid"
      : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
      : "a"(leaf)
    );
  }

  void IA32CPUDriver::_initializePAT() {
    UInt32 eax, ebx, ecx, edx;

    _executeCPUID(1, eax, ebx, ecx, edx);

    if (!(edx & (1u << 16))) {
      KLOG_INFO("PAT not supported, WriteCombining unavailable");

      return;
    } else {
      constexpr UInt32 PAT_MSR = 0x277;

      UInt64 pat = ReadMSR(PAT_MSR);

      pat &= ~(static_cast<UInt64>(0xFF) << 32);
      pat |= static_cast<UInt64>(0x01) << 32;

      WriteMSR(PAT_MSR, pat);
      _patSupported = true;

      KLOG_TRACE("PAT initialized (entry 4 as WriteCombining)");
    }
  }
}
