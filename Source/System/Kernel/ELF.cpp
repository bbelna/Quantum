/**
 * @file System/Kernel/ELF.cpp
 * @brief Minimal ELF loader helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright Ac 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Align.hpp>
#include <Bytes.hpp>
#include <Types.hpp>

#include "Arch/AddressSpace.hpp"
#include "Arch/PhysicalAllocator.hpp"
#include "ELF.hpp"

namespace Quantum::System::Kernel {
  using ::Quantum::AlignDown;
  using ::Quantum::AlignUp;
  using ::Quantum::CopyBytes;

  bool ELF::ValidateHeader(const UInt8* image, UInt32 size) {
    if (!image || size < sizeof(ELFHeader32)) {
      return false;
    }

    const ELFHeader32* header
      = reinterpret_cast<const ELFHeader32*>(image);

    for (UInt32 i = 0; i < 4; ++i) {
      if (header->ident[i] != _elfMagic[i]) {
        return false;
      }
    }

    if (
      header->ident[4] != _elfClass32
      || header->ident[5] != _elfData2LSB
      || header->ident[6] != _elfVersion
      || header->programHeaderEntrySize < sizeof(ELFProgramHeader32)
    ) {
      return false;
    }

    UInt32 phBytes = static_cast<UInt32>(header->programHeaderEntrySize)
      * static_cast<UInt32>(header->programHeaderCount);
    UInt32 phEnd = header->programHeaderOffset + phBytes;

    if (header->programHeaderOffset >= size || phEnd > size) {
      return false;
    }

    return true;
  }

  bool ELF::IsValid(const UInt8* image, UInt32 size) {
    return ValidateHeader(image, size);
  }

  bool ELF::LoadUserImage(
    const UInt8* image,
    UInt32 size,
    UInt32 addressSpace,
    UInt32& outEntry,
    UInt32& outImageEnd
  ) {
    outEntry = 0;
    outImageEnd = 0;

    if (!ValidateHeader(image, size)) {
      return false;
    }

    const ELFHeader32* header
      = reinterpret_cast<const ELFHeader32*>(image);

    bool loaded = false;
    UInt32 imageEnd = 0;

    for (UInt32 i = 0; i < header->programHeaderCount; ++i) {
      UInt32 offset
        = header->programHeaderOffset + i * header->programHeaderEntrySize;
      const ELFProgramHeader32* phdr
        = reinterpret_cast<const ELFProgramHeader32*>(image + offset);

      if (
        phdr->segmentType != _ptLoad
        || phdr->memorySize == 0
      ) {
        continue;
      }

      if (phdr->fileOffset + phdr->fileSize > size) {
        return false;
      }

      UInt32 segmentStart = phdr->virtualAddress;
      UInt32 segmentEnd = phdr->virtualAddress + phdr->memorySize;

      if (segmentEnd < segmentStart) {
        return false;
      }

      UInt32 pageStart = AlignDown(segmentStart, _pageSize);
      UInt32 pageEnd = AlignUp(segmentEnd, _pageSize);
      bool writable = (phdr->segmentFlags & _pfWrite) != 0;

      for (UInt32 vaddr = pageStart; vaddr < pageEnd; vaddr += _pageSize) {
        UInt32 phys = Arch::PhysicalAllocator::AllocatePage(true);

        if (phys == 0) {
          return false;
        }

        Arch::AddressSpace::MapPage(
          addressSpace,
          vaddr,
          phys,
          writable,
          true,
          false
        );

        UInt8* dest = reinterpret_cast<UInt8*>(phys);

        for (UInt32 j = 0; j < _pageSize; ++j) {
          dest[j] = 0;
        }

        UInt32 copyStart = vaddr < segmentStart ? segmentStart : vaddr;
        UInt32 copyEnd = vaddr + _pageSize;
        UInt32 fileEnd = segmentStart + phdr->fileSize;

        if (copyEnd > fileEnd) {
          copyEnd = fileEnd;
        }

        if (copyStart < copyEnd) {
          UInt32 copyBytes = copyEnd - copyStart;
          UInt32 sourceOffset
            = phdr->fileOffset + (copyStart - segmentStart);
          UInt32 destOffset = copyStart - vaddr;

          CopyBytes(dest + destOffset, image + sourceOffset, copyBytes);
        }
      }

      if (segmentEnd > imageEnd) {
        imageEnd = segmentEnd;
      }

      loaded = true;
    }

    if (!loaded) {
      return false;
    }

    outEntry = header->entryAddress;
    outImageEnd = imageEnd;

    return true;
  }
}
