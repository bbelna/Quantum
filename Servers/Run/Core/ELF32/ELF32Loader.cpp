/**
 * @file Servers/Run/Core/ELF32/ELF32Loader.cpp
 * @brief Implements @ref @QRunSrv::ELF32::ELF32Loader.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <RunServerTypes.hpp>

#include "ELF32Loader.hpp"
#include "GetELF32ProgramHeader.hpp"
#include "ValidateELF32Header.hpp"

namespace Quantum::Servers::Run::Core::ELF32 {
  ProcessID ELF32Loader::Load(
    const char* name,
    const UInt8* image,
    Size size,
    Size argumentCount,
    const char* argumentData,
    Size argumentDataSize,
    UInt8 streamCount,
    const SharedBufferID* streamBufferIDs
  ) {
    static constexpr Size maxSegments = 8;

    // validate the ELF header
    if (!ValidateELF32Header(image, size)) {
      return InvalidProcessID;
    }

    const ELF32Header* header = Cast::As<ELF32Header>(image);
    UInt32 loadBase = 0xFFFFFFFF;
    UInt32 loadEnd = 0;
    bool hasLoadable = false;

    for (
      UInt32 programHeaderIndex = 0;
      programHeaderIndex < header->ProgramHeaderCount;
      ++programHeaderIndex
    ) {
      const ELF32ProgramHeader* programHeader = GetELF32ProgramHeader(
        image,
        programHeaderIndex
      );

      if (
        programHeader->SegmentType == ELFSegmentType::Load &&
        programHeader->MemorySizeInBytes > 0
      ) {
        UInt32 segmentStart = programHeader->VirtualAddress;
        UInt32 segmentEnd = segmentStart + programHeader->MemorySizeInBytes;

        // check for overflow, reject segments that overlap kernel space;
        // ELF spec requires FileSize <= MemorySize for PT_LOAD; a crafted
        // binary violating this would overwrite past the segment's region
        if (
          segmentEnd >= segmentStart &&
          segmentStart < KernelBase &&
          segmentEnd <= KernelBase &&
          programHeader->FileSizeInBytes <= programHeader->MemorySizeInBytes
        ) {
          // validate file bounds
          if (
              static_cast<Size>(programHeader->FileOffset)
            + static_cast<Size>(programHeader->FileSizeInBytes)
            > size
          ) {
            return InvalidProcessID;
          }

          if (segmentStart < loadBase) {
            loadBase = segmentStart;
          }

          if (segmentEnd > loadEnd) {
            loadEnd = segmentEnd;
          }

          hasLoadable = true;
        } else {
          return InvalidProcessID;
        }
      }
    }

    if (hasLoadable) {
      // reject entry points outside the loadable range or in kernel space
      UInt32 entry = header->EntryAddress;

      if (
        entry >= loadBase &&
        entry < loadEnd &&
        entry < KernelBase
      ) {
        // page-align the span
        loadBase = AlignDown(
          loadBase,
          BlockSize
        );
        loadEnd = AlignUp(
          loadEnd,
          BlockSize
        );

        Size totalSpan = loadEnd - loadBase;
        UIntPtr buffer = AllocateBlock(totalSpan);

        if (buffer != 0) {
          // zero the entire buffer (handles BSS regions)
          Byte::Zero(
            reinterpret_cast<void*>(buffer),
            totalSpan
          );

          // copy each PT_LOAD segment into the buffer at its correct offset and
          // build an array of segment descriptors with per-segment permissions
          ProcessSpawnSegment segments[maxSegments];
          Size segmentCount = 0;

          for (
            UInt32 programHeaderIndex = 0;
            programHeaderIndex < header->ProgramHeaderCount;
            ++programHeaderIndex
          ) {
            const ELF32ProgramHeader* programHeader = GetELF32ProgramHeader(
              image,
              programHeaderIndex
            );

            if (
              programHeader->SegmentType == ELFSegmentType::Load &&
              programHeader->MemorySizeInBytes > 0
            ) {
              UInt32 offset
                = programHeader->VirtualAddress
                - loadBase;

              Byte::Copy(
                reinterpret_cast<void*>(buffer + offset),
                image + programHeader->FileOffset,
                programHeader->FileSizeInBytes
              );

              // translate ELF segment flags to ABI permission flags
              if (segmentCount < maxSegments) {
                UInt32 permissions = 0;

                if (
                  Enum::HasFlag(
                    programHeader->SegmentFlags,
                    ELFSegmentFlags::Read
                  )
                ) {
                  permissions |= 1;
                }

                if (
                  Enum::HasFlag(
                    programHeader->SegmentFlags,
                    ELFSegmentFlags::Write
                  )
                ) {
                  permissions |= 2;
                }

                if (
                  Enum::HasFlag(
                    programHeader->SegmentFlags,
                    ELFSegmentFlags::Execute
                  )
                ) {
                  permissions |= 4;
                }

                segments[segmentCount++] = ProcessSpawnSegment {
                  static_cast<Size>(offset),
                  static_cast<Size>(programHeader->MemorySizeInBytes),
                  permissions
                };
              }
            }
          }

          // spawn the process with the prepared buffer and segment
          // permissions
          ProcessSpawnParameters params {
            buffer,
            totalSpan,
            loadBase,
            segmentCount,
            segments,
            argumentCount,
            argumentData,
            argumentDataSize,
            streamCount,
            {}
          };

          if (streamBufferIDs) {
            for (
              UInt8 streamIndex = 0;
              streamIndex < streamCount && streamIndex < 3;
              streamIndex++
            ) {
              params.StreamBufferIDs[streamIndex]
                = streamBufferIDs[streamIndex];
            }
          }

          const char* shortName = name;

          for (
            const char* p = name;
            *p;
            ++p
          ) {
            if (*p == '/') {
              shortName = p + 1;
            }
          }

          ProcessID pid = Kernel.SpawnProcess(
            shortName,
            header->EntryAddress,
            &params
          );

          // the kernel retains (ref-counts) each page mapped into the
          // child, so freeing the RunServer's mapping is safe, the
          // child's pages survive
          FreeBlock(buffer);

          return pid;
        }
      }
    }

    return InvalidProcessID;
  }
}
