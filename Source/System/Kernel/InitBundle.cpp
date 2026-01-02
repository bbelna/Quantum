/**
 * @file System/Kernel/InitBundle.cpp
 * @brief Init bundle handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Align.hpp>
#include <Types.hpp>

#include "Arch/AddressSpace.hpp"
#include "Arch/Paging.hpp"
#include "Arch/PhysicalAllocator.hpp"
#include "BootInfo.hpp"
#include "ELF.hpp"
#include "InitBundle.hpp"
#include "Logger.hpp"
#include "Prelude.hpp"
#include "Task.hpp"
#include "UserMode.hpp"

namespace Quantum::System::Kernel {
  using ::Quantum::AlignUp;

  using BundleHeader = ABI::InitBundle::Header;
  using BundleEntry = ABI::InitBundle::Entry;
  using BundleEntryType = ABI::InitBundle::EntryType;
  using LogLevel = Kernel::Logger::Level;

  namespace {
    struct LoadedImage {
      UInt32 entry;
      UInt32 imageEnd;
    };

    bool LoadLegacyImage(
      const UInt8* payload,
      UInt32 size,
      UInt32 addressSpace,
      UInt32 programBase,
      UInt32 stackTop,
      LoadedImage& out
    ) {
      if (!payload || size < sizeof(UInt32)) {
        return false;
      }

      UInt32 entryOffset = *reinterpret_cast<const UInt32*>(payload);
      UInt32 imageBytes = size;
      UInt32 maxImageBytes = stackTop - programBase;

      if (size >= sizeof(UInt32) * 2) {
        UInt32 reportedBytes
          = *reinterpret_cast<const UInt32*>(payload + sizeof(UInt32));

        if (reportedBytes >= size && reportedBytes <= maxImageBytes) {
          imageBytes = reportedBytes;
        }
      }

      if (entryOffset >= size) {
        return false;
      }

      constexpr UInt32 pageSize = 4096;
      UInt32 pages = AlignUp(imageBytes, pageSize) / pageSize;

      for (UInt32 i = 0; i < pages; ++i) {
        UInt32 phys = Arch::PhysicalAllocator::AllocatePage(true);
        UInt32 vaddr = programBase + i * pageSize;

        if (phys == 0) {
          return false;
        }

        Arch::AddressSpace::MapPage(
          addressSpace,
          vaddr,
          phys,
          true,
          true,
          false
        );

        UInt32 offset = i * pageSize;

        if (offset < size) {
          UInt32 toCopy = size - offset;

          if (toCopy > pageSize) {
            toCopy = pageSize;
          }

          UInt8* dest = reinterpret_cast<UInt8*>(phys);

          for (UInt32 j = 0; j < toCopy; ++j) {
            dest[j] = payload[offset + j];
          }
        }
      }

      out.entry = programBase + entryOffset;
      out.imageEnd = programBase + imageBytes;

      return true;
    }

    bool LoadBundleImage(
      const UInt8* payload,
      UInt32 size,
      UInt32 addressSpace,
      UInt32 programBase,
      UInt32 stackTop,
      LoadedImage& out
    ) {
      if (
        ELF::LoadUserImage(
          payload,
          size,
          addressSpace,
          out.entry,
          out.imageEnd
        )
      ) {
        return true;
      }

      return LoadLegacyImage(
        payload,
        size,
        addressSpace,
        programBase,
        stackTop,
        out
      );
    }
  }

  void InitBundle::MapInitBundle() {
    BootInfo::InitBundleInfo initBundle {};

    if (!BootInfo::GetInitBundleInfo(initBundle)) {
      return;
    }

    UInt32 size = initBundle.size;
    UInt32 base = initBundle.physical;

    Logger::WriteFormatted(
      LogLevel::Debug,
      "INIT.BND boot info: phys=%p size=0x%x",
      base,
      size
    );

    UInt32 pageCount = AlignUp(size, 4096) / 4096;

    for (UInt32 i = 0; i < pageCount; ++i) {
      UInt32 phys = base + i * 4096;
      UInt32 virt = _initBundleVirtualBase + i * 4096;

      Arch::Paging::MapPage(virt, phys, false, false, false);
      Arch::Paging::MapPage(
        _initBundleUserBase + i * 4096,
        phys,
        false,
        true,
        true
      );
    }

    _initBundleMappedBase = _initBundleVirtualBase;
    _initBundleMappedSize = size;
    _initBundleMappedUserBase = _initBundleUserBase;

    if (size >= 8) {
      const UInt8* physMagic = reinterpret_cast<const UInt8*>(base);
      const UInt8* virtMagic
        = reinterpret_cast<const UInt8*>(_initBundleMappedBase);
      UInt32 physMagic0 = *reinterpret_cast<const UInt32*>(physMagic);
      UInt32 physMagic1 = *reinterpret_cast<const UInt32*>(physMagic + 4);
      UInt32 virtMagic0 = *reinterpret_cast<const UInt32*>(virtMagic);
      UInt32 virtMagic1 = *reinterpret_cast<const UInt32*>(virtMagic + 4);

      Logger::WriteFormatted(
        LogLevel::Debug,
        "INIT.BND post-map: phys=%p magic0=0x%x magic1=0x%x",
        base,
        physMagic0,
        physMagic1
      );
      Logger::WriteFormatted(
        LogLevel::Debug,
        "INIT.BND post-map: virt=%p magic0=0x%x magic1=0x%x",
        _initBundleMappedBase,
        virtMagic0,
        virtMagic1
      );
    }
  }

  bool InitBundle::HasMagic(const BundleHeader& header) {
    const char expected[8] = { 'I','N','I','T','B','N','D','\0' };

    for (UInt32 i = 0; i < 8; ++i) {
      if (header.magic[i] != expected[i]) {
        return false;
      }
    }

    return true;
  }

  const BundleEntry* InitBundle::GetBundleEntries(
    UInt32& entryCount
  ) {
    entryCount = 0;

    if (_initBundleMappedSize < sizeof(BundleHeader)) {
      return nullptr;
    }

    const UInt8* base = reinterpret_cast<const UInt8*>(_initBundleMappedBase);
    const BundleHeader* header
      = reinterpret_cast<const BundleHeader*>(base);

    if (!HasMagic(*header)) {
      UInt32 magic0 = *reinterpret_cast<const UInt32*>(header->magic);
      UInt32 magic1 = *reinterpret_cast<const UInt32*>(header->magic + 4);

      Logger::WriteFormatted(
        LogLevel::Debug,
        "INIT.BND bad magic: magic0=0x%x magic1=0x%x ver=%u entries=%u "
          "table=0x%x",
        magic0,
        magic1,
        static_cast<UInt32>(header->version),
        static_cast<UInt32>(header->entryCount),
        header->tableOffset
      );

      char magic[9];

      for (UInt32 i = 0; i < 8; ++i) {
        magic[i] = header->magic[i];
      }

      magic[8] = '\0';

      Logger::WriteFormatted(
        LogLevel::Warning,
        "INIT.BND magic mismatch (%s)",
        magic
      );

      return nullptr;
    }

    UInt32 tableOffset = header->tableOffset;

    entryCount = header->entryCount;

    UInt32 tableBytes = entryCount * static_cast<UInt32>(sizeof(BundleEntry));

    if (tableOffset + tableBytes > _initBundleMappedSize) {
      Logger::WriteFormatted(
        LogLevel::Warning,
        "INIT.BND table out of range (off=0x%X count=%u size=0x%X total=0x%X)",
        tableOffset,
        entryCount,
        tableBytes,
        _initBundleMappedSize
      );

      return nullptr;
    }

    return reinterpret_cast<const BundleEntry*>(base + tableOffset);
  }

  const BundleEntry* InitBundle::FindCoordinatorEntry() {
    UInt32 entryCount = 0;
    const BundleEntry* entries = GetBundleEntries(entryCount);

    if (!entries) {
      return nullptr;
    }

    for (UInt32 i = 0; i < entryCount; ++i) {
      if (entries[i].type == BundleEntryType::Init) {
        return &entries[i];
      }
    }

    Logger::WriteFormatted(
      LogLevel::Warning,
      "INIT.BND has %u entries but no init type",
      entryCount
    );

    return nullptr;
  }

  bool InitBundle::EntryNameMatches(const BundleEntry& entry, CString name) {
    if (!name) {
      return false;
    }

    for (UInt32 i = 0; i < 32; ++i) {
      char entryChar = entry.name[i];
      char nameChar = name[i];

      if (entryChar != nameChar) {
        return false;
      }

      if (entryChar == '\0') {
        return true;
      }
    }

    return false;
  }

  const BundleEntry* InitBundle::FindEntryByName(CString name) {
    UInt32 entryCount = 0;
    const BundleEntry* entries = GetBundleEntries(entryCount);

    if (!entries) {
      return nullptr;
    }

    for (UInt32 i = 0; i < entryCount; ++i) {
      if (EntryNameMatches(entries[i], name)) {
        return &entries[i];
      }
    }

    return nullptr;
  }

  void InitBundle::Initialize() {
    MapInitBundle();
  }

  void InitBundle::LaunchCoordinatorTask() {
    if (_initBundleMappedSize == 0 || _initBundleMappedBase == 0) {
      Logger::Write(LogLevel::Warning, "INIT.BND not mapped");
      Task::Exit();
    }

    const BundleEntry* entry = FindCoordinatorEntry();

    if (!entry) {
      Logger::Write(LogLevel::Warning, "Coordinator entry not found");
      Task::Exit();
    }

    if (entry->offset + entry->size > _initBundleMappedSize) {
      Logger::Write(LogLevel::Warning, "Coordinator entry out of range");
      Task::Exit();
    }

    constexpr UInt32 pageSize = 4096;
    const UInt8* bundleBase
      = reinterpret_cast<const UInt8*>(_initBundleMappedBase);
    const UInt8* payload = bundleBase + entry->offset;
    UInt32 size = entry->size;

    UInt32 addressSpace = Arch::AddressSpace::Create();

    if (addressSpace == 0) {
      Logger::Write(LogLevel::Warning, "Failed to create address space");
      Task::Exit();
    }

    LoadedImage loaded {};

    if (
      !LoadBundleImage(
        payload,
        size,
        addressSpace,
        _userProgramBase,
        _userStackTop,
        loaded
      )
    ) {
      Logger::Write(LogLevel::Warning, "Coordinator image load failed");
      Arch::AddressSpace::Destroy(addressSpace);
      Task::Exit();
    }

    UInt32 stackBytes = AlignUp(_userStackSize, pageSize);
    UInt32 stackBase = _userStackTop - stackBytes;
    UInt32 stackPages = stackBytes / pageSize;

    for (UInt32 i = 0; i < stackPages; ++i) {
      UInt32 phys = Arch::PhysicalAllocator::AllocatePage(true);
      UInt32 vaddr = stackBase + i * pageSize;

      Arch::AddressSpace::MapPage(
        addressSpace,
        vaddr,
        phys,
        true,
        true,
        false
      );
    }

    Task::SetCoordinatorId(Task::GetCurrentId());
    Task::SetCurrentAddressSpace(addressSpace);
    Arch::AddressSpace::Activate(addressSpace);
    UserMode::Enter(
      loaded.entry,
      _userStackTop
    );
  }

  bool InitBundle::GetInfo(UInt32& base, UInt32& size) {
    if (_initBundleMappedSize == 0 || _initBundleMappedUserBase == 0) {
      base = 0;
      size = 0;

      return false;
    }

    base = _initBundleMappedUserBase;
    size = _initBundleMappedSize;

    return true;
  }

  UInt32 InitBundle::SpawnTask(CString name) {
    if (_initBundleMappedSize == 0 || _initBundleMappedBase == 0) {
      Logger::Write(LogLevel::Warning, "SpawnTask: INIT.BND not mapped");

      return 0;
    }

    const BundleEntry* entry = FindEntryByName(name);

    if (!entry) {
      Logger::Write(LogLevel::Warning, "SpawnTask: entry not found");

      return 0;
    }

    if (entry->offset + entry->size > _initBundleMappedSize) {
      Logger::WriteFormatted(
        LogLevel::Warning,
        "SpawnTask: entry out of range (off=%p size=%p total=%p)",
        entry->offset,
        entry->size,
        _initBundleMappedSize
      );
      Logger::Write(LogLevel::Warning, "SpawnTask: entry out of range");

      return 0;
    }

    const UInt8* bundleBase
      = reinterpret_cast<const UInt8*>(_initBundleMappedBase);
    const UInt8* payload = bundleBase + entry->offset;
    UInt32 size = entry->size;

    return SpawnImage(payload, size);
  }

  UInt32 InitBundle::SpawnImage(const UInt8* image, UInt32 size) {
    if (!image || size == 0) {
      Logger::Write(LogLevel::Warning, "SpawnTask: image payload invalid");

      return 0;
    }

    constexpr UInt32 pageSize = 4096;
    UInt32 addressSpace = Arch::AddressSpace::Create();

    if (addressSpace == 0) {
      Logger::Write(
        LogLevel::Warning,
        "SpawnTask: failed to create address space"
      );

      return 0;
    }

    LoadedImage loaded {};

    if (
      !LoadBundleImage(
        image,
        size,
        addressSpace,
        _userProgramBase,
        _userStackTop,
        loaded
      )
    ) {
      Logger::Write(LogLevel::Warning, "SpawnTask: image load failed");
      Arch::AddressSpace::Destroy(addressSpace);

      return 0;
    }

    UInt32 stackBytes = AlignUp(_userStackSize, pageSize);
    UInt32 stackBase = _userStackTop - stackBytes;
    UInt32 stackPages = stackBytes / pageSize;

    for (UInt32 i = 0; i < stackPages; ++i) {
      UInt32 phys = Arch::PhysicalAllocator::AllocatePage(true);
      UInt32 vaddr = stackBase + i * pageSize;

      Arch::AddressSpace::MapPage(
        addressSpace,
        vaddr,
        phys,
        true,
        true,
        false
      );
    }

    Task::ControlBlock* task = Task::CreateUser(
      loaded.entry,
      _userStackTop,
      addressSpace
    );

    if (!task) {
      Logger::Write(LogLevel::Warning, "SpawnTask: failed to create task");
      Arch::AddressSpace::Destroy(addressSpace);

      return 0;
    }

    // initialize per-task heap bounds
    UInt32 heapBase = AlignUp(loaded.imageEnd, pageSize);
    UInt32 heapLimit = stackBase;

    if (heapBase < heapLimit) {
      task->userHeapBase = heapBase;
      task->userHeapEnd = heapBase;
      task->userHeapMappedEnd = heapBase;
      task->userHeapLimit = heapLimit;
    } else {
      Logger::Write(LogLevel::Warning, "SpawnTask: no space for user heap");
    }

    return task->id;
  }
}
