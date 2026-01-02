/**
 * @file System/Kernel/Include/ELF.hpp
 * @brief Minimal ELF loader helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright Ac 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel {
  /**
   * Minimal ELF loader helpers.
   */
  class ELF {
    public:
      /**
       * Checks whether the image appears to be a valid ELF32 binary.
       * @param image
       *   Pointer to the ELF image.
       * @param size
       *   Size of the image in bytes.
       * @return
       *   True if the image is an ELF32 binary; false otherwise.
       */
      static bool IsValid(const UInt8* image, UInt32 size);

      /**
       * Loads an ELF32 user image into the specified address space.
       * @param image
       *   Pointer to the ELF image.
       * @param size
       *   Size of the image in bytes.
       * @param addressSpace
       *   Target address space page directory physical address.
       * @param outEntry
       *   Receives the user entry point.
       * @param outImageEnd
       *   Receives the end of the mapped image.
       * @return
       *   True on success; false on failure.
       */
      static bool LoadUserImage(
        const UInt8* image,
        UInt32 size,
        UInt32 addressSpace,
        UInt32& outEntry,
        UInt32& outImageEnd
      );

    private:
      /**
       * ELF32 header descriptor.
       */
      struct ELFHeader32 {
        /**
         * ELF identification bytes.
         */
        UInt8 ident[16];

        /**
         * ELF object file type.
         */
        UInt16 fileType;

        /**
         * Target machine architecture.
         */
        UInt16 machine;

        /**
         * ELF version.
         */
        UInt32 version;

        /**
         * Entry point virtual address.
         */
        UInt32 entryAddress;

        /**
         * Program header table offset.
         */
        UInt32 programHeaderOffset;

        /**
         * Section header table offset.
         */
        UInt32 sectionHeaderOffset;

        /**
         * Processor-specific flags.
         */
        UInt32 flags;

        /**
         * ELF header size in bytes.
         */
        UInt16 headerSize;

        /**
         * Program header entry size in bytes.
         */
        UInt16 programHeaderEntrySize;

        /**
         * Program header entry count.
         */
        UInt16 programHeaderCount;

        /**
         * Section header entry size in bytes.
         */
        UInt16 sectionHeaderEntrySize;

        /**
         * Section header entry count.
         */
        UInt16 sectionHeaderCount;

        /**
         * Section header string table index.
         */
        UInt16 sectionHeaderStringIndex;
      };

      /**
       * ELF32 program header descriptor.
       */
      struct ELFProgramHeader32 {
        /**
         * Segment type.
         */
        UInt32 segmentType;

        /**
         * Segment file offset.
         */
        UInt32 fileOffset;

        /**
         * Segment virtual address.
         */
        UInt32 virtualAddress;

        /**
         * Segment physical address.
         */
        UInt32 physicalAddress;

        /**
         * Segment file size in bytes.
         */
        UInt32 fileSize;

        /**
         * Segment memory size in bytes.
         */
        UInt32 memorySize;

        /**
         * Segment flags.
         */
        UInt32 segmentFlags;

        /**
         * Segment alignment.
         */
        UInt32 alignment;
      };

      /**
       * ELF magic bytes.
       */
      inline static constexpr UInt8 _elfMagic[4] = { 0x7F, 'E', 'L', 'F' };

      /**
       * ELF class identifier for 32-bit objects.
       */
      static constexpr UInt8 _elfClass32 = 1;

      /**
       * ELF data identifier for little endian.
       */
      static constexpr UInt8 _elfData2LSB = 1;

      /**
       * ELF version identifier.
       */
      static constexpr UInt8 _elfVersion = 1;

      /**
       * ELF program header type for loadable segments.
       */
      static constexpr UInt32 _ptLoad = 1;

      /**
       * ELF program header flag for writable segments.
       */
      static constexpr UInt32 _pfWrite = 1u << 1;

      /**
       * ELF loader page size.
       */
      static constexpr UInt32 _pageSize = 4096;

      /**
       * Validates the ELF header.
       * @param image
       *   Pointer to the ELF image.
       * @param size
       *   Size of the image in bytes.
       * @return
       *   True if the header appears valid; false otherwise.
       */
      static bool ValidateHeader(const UInt8* image, UInt32 size);
  };
}
