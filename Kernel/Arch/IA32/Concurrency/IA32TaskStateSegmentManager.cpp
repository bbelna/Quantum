/**
 * @file Kernel/Arch/IA32/Concurrency/IA32TaskStateSegmentManager.cpp
 * @brief Implements @ref @QKrnlIA32::Concurrency::TaskStateSegmentManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Arch/IA32/Memory/IA32GDT.hpp>
#include <Arch/IA32/Memory/IA32MemoryTypes.hpp>
#include <KernelLog.hpp>

#include "IA32TaskStateSegmentManager.hpp"

namespace Quantum::Kernel::Arch::IA32::Concurrency {
  /**
   * @brief GDT index for the TSS descriptor.
   */
  constexpr Size TSSGDTIndex = 5;

  IA32TaskStateSegmentManager::IA32TaskStateSegmentManager(
    UIntPtr kernelStackTop
  ) {
    // zero out the entire TSS
    CString::Set(&_tss, 0, sizeof(IA32TaskStateSegment));

    // set up ring 0 stack
    _tss.SS0 = KernelDataSelector;
    _tss.ESP0 = kernelStackTop;

    // set I/O map base to end of TSS (no I/O bitmap)
    _tss.IOMapBase = sizeof(IA32TaskStateSegment);

    _initializeDescriptor();

    KLOG_TRACE(
      "TSS initialized at %p, ESP0 %p, loaded TR %x",
      &_tss,
      kernelStackTop,
      TSSSelector
    );
  }

  void IA32TaskStateSegmentManager::SetKernelStack(UIntPtr stackTop) {
    _tss.ESP0 = stackTop;

    // keep SYSENTER_ESP in sync with TSS.ESP0 so that sysenter lands on
    // the correct kernel stack for the current thread
    asm volatile(
      "wrmsr"
      :
      : "c"(0x175u), "a"(stackTop), "d"(0u)
      : "memory"
    );
  }

  UIntPtr IA32TaskStateSegmentManager::GetKernelStack() const {
    return _tss.ESP0;
  }

  void IA32TaskStateSegmentManager::_initializeDescriptor() {
    UInt32 tssBase = reinterpret_cast<UInt32>(&_tss);
    UInt32 tssLimit = sizeof(IA32TaskStateSegment) - 1;

    IA32GDTEntry& tssDescriptor = IA32GDT[TSSGDTIndex];

    tssDescriptor.LimitLow = tssLimit & 0xFFFF;
    tssDescriptor.BaseLow = tssBase & 0xFFFF;
    tssDescriptor.BaseMid = (tssBase >> 16) & 0xFF;
    tssDescriptor.Access = 0x89;
    tssDescriptor.Granularity = (tssLimit >> 16) & 0x0F;
    tssDescriptor.BaseHigh = (tssBase >> 24) & 0xFF;

    // load the task register
    asm volatile("ltr %0" : : "r"(TSSSelector) : "memory");
  }
}
