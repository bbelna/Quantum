/**
 * @file System/Kernel/AddressSpace.cpp
 * @brief Address space and page mapping helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include "AddressSpace.hpp"
#include "Arch/Memory.hpp"

namespace Quantum::System::Kernel {
  UInt32 AddressSpace::GetKernelPageDirectoryPhysical() {
    return Arch::Memory::GetKernelPageDirectoryPhysical();
  }

  void AddressSpace::MapPage(
    UInt32 virtualAddress,
    UInt32 physicalAddress,
    bool writable,
    bool user,
    bool global
  ) {
    Arch::Memory::MapPage(
      virtualAddress,
      physicalAddress,
      writable,
      user,
      global
    );
  }

  UInt32 AddressSpace::Create() {
    return Arch::Memory::CreateAddressSpace();
  }

  void AddressSpace::Destroy(UInt32 pageDirectoryPhysical) {
    Arch::Memory::DestroyAddressSpace(pageDirectoryPhysical);
  }

  void AddressSpace::MapPageInAddressSpace(
    UInt32 pageDirectoryPhysical,
    UInt32 virtualAddress,
    UInt32 physicalAddress,
    bool writable,
    bool user,
    bool global
  ) {
    Arch::Memory::MapPageInAddressSpace(
      pageDirectoryPhysical,
      virtualAddress,
      physicalAddress,
      writable,
      user,
      global
    );
  }

  void AddressSpace::Activate(UInt32 pageDirectoryPhysical) {
    Arch::Memory::ActivateAddressSpace(pageDirectoryPhysical);
  }
}
