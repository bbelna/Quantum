/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Kernel.cpp
 * Core kernel implementation.
 */

#include <BootInfo.hpp>
#include <CPU.hpp>
#include <Handlers/PanicHandler.hpp>
#include <Handlers/SystemCallHandler.hpp>
#include <Helpers/CStringHelper.hpp>
#include <Helpers/DebugHelper.hpp>
#include <Interrupts.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Interrupts.hpp>
#include <Memory.hpp>
#include <Task.hpp>
#include <Testing.hpp>
#include <UserMode.hpp>
#include <Types.hpp>

namespace Quantum::System::Kernel {
  using CStringHelper = Helpers::CStringHelper;
  using LogLevel = Logger::Level;

  namespace {
    constexpr UInt32 _initBundleVirtualBase = 0xC1000000;
    constexpr UInt32 _initBundleUserBase = 0x00900000;
    constexpr UInt32 _userProgramBase = 0x00400000;
    constexpr UInt32 _userStackTop = 0x00800000;
    constexpr UInt32 _userStackSize = 16 * 4096;

    UInt32 _initBundleMappedBase = 0;
    UInt32 _initBundleMappedSize = 0;
    UInt32 _initBundleMappedUserBase = 0;

    UInt32 AlignUp(UInt32 value, UInt32 alignment) {
      return (value + alignment - 1) & ~(alignment - 1);
    }

    void MapInitBundle() {
      BootInfo::InitBundleInfo initBundle{};

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

        Memory::MapPage(virt, phys, false, false, false);
        Memory::MapPage(
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

    struct BundleHeader {
      char magic[8];
      UInt16 version;
      UInt16 entryCount;
      UInt32 tableOffset;
      UInt8 reserved[8];
    };

    struct BundleEntry {
      char name[32];
      UInt8 type;
      UInt8 flags;
      UInt8 reserved[2];
      UInt32 offset;
      UInt32 size;
      UInt32 checksum;
    };

    bool HasMagic(const BundleHeader& header) {
      const char expected[8] = { 'I','N','I','T','B','N','D','\0' };

      for (UInt32 i = 0; i < 8; ++i) {
        if (header.magic[i] != expected[i]) {
          return false;
        }
      }

      return true;
    }

    const BundleEntry* GetBundleEntries(UInt32& entryCount) {
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

    const BundleEntry* FindCoordinatorEntry() {
      UInt32 entryCount = 0;
      const BundleEntry* entries = GetBundleEntries(entryCount);

      if (!entries) {
        return nullptr;
      }

      for (UInt32 i = 0; i < entryCount; ++i) {
        if (entries[i].type == 1) {
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

    bool EntryNameMatches(const BundleEntry& entry, CString name) {
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

    const BundleEntry* FindEntryByName(CString name) {
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

    void LaunchCoordinatorFromBundle() {
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

      const UInt8* bundleBase
        = reinterpret_cast<const UInt8*>(_initBundleMappedBase);
      const UInt8* payload = bundleBase + entry->offset;
      UInt32 size = entry->size;
      constexpr UInt32 pageSize = 4096;

      if (size < sizeof(UInt32)) {
        Logger::Write(LogLevel::Warning, "Coordinator payload too small");
        Task::Exit();
      }

      UInt32 entryOffset = *reinterpret_cast<const UInt32*>(payload);
      UInt32 addressSpace = Memory::CreateAddressSpace();

      if (addressSpace == 0) {
        Logger::Write(LogLevel::Warning, "Failed to create address space");
        Task::Exit();
      }

      UInt32 pages = AlignUp(size, pageSize) / pageSize;

      for (UInt32 i = 0; i < pages; ++i) {
        void* phys = Memory::AllocatePage(true);
        UInt32 vaddr = _userProgramBase + i * pageSize;

        Memory::MapPageInAddressSpace(
          addressSpace,
          vaddr,
          reinterpret_cast<UInt32>(phys),
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

      UInt32 stackBytes = AlignUp(_userStackSize, pageSize);
      UInt32 stackBase = _userStackTop - stackBytes;
      UInt32 stackPages = stackBytes / pageSize;

      for (UInt32 i = 0; i < stackPages; ++i) {
        void* phys = Memory::AllocatePage(true);
        UInt32 vaddr = stackBase + i * pageSize;

        Memory::MapPageInAddressSpace(
          addressSpace,
          vaddr,
          reinterpret_cast<UInt32>(phys),
          true,
          true,
          false
        );
      }

      Task::SetCoordinatorId(Task::GetCurrentId());
      Task::SetCurrentAddressSpace(addressSpace);
      Memory::ActivateAddressSpace(addressSpace);
      UserMode::Enter(
        _userProgramBase + entryOffset,
        _userStackTop
      );
    }

    #ifdef KERNEL_TESTS
    /**
     * Kernel test runner task entry point.
     */
    void KernelTestRunner() {
      Testing::RegisterBuiltins();
      Testing::RunAll();
      Logger::Write(LogLevel::Info, "Kernel tests task finished");
      Task::Exit();
    }
    #endif
  }

  void Initialize(UInt32 bootInfoPhysicalAddress) {
    BootInfo::Initialize(bootInfoPhysicalAddress);
    Memory::Initialize(bootInfoPhysicalAddress);
    Interrupts::Initialize();
    Task::Initialize();
    Task::EnablePreemption();
    MapInitBundle();

    Task::Create(LaunchCoordinatorFromBundle, 4096);

    #ifdef KERNEL_TESTS
    // spawn test runner task and start scheduling
    Task::Create(KernelTestRunner, 4096);
    #endif

    // enter scheduler; if no tests are queued we fall back to idle
    Task::Yield();
  }

  Interrupts::Context* HandleSystemCall(Interrupts::Context& context) {
    return Handlers::SystemCallHandler::Handle(context);
  }

  bool GetInitBundleInfo(UInt32& base, UInt32& size) {
    if (_initBundleMappedSize == 0 || _initBundleMappedUserBase == 0) {
      base = 0;
      size = 0;

      return false;
    }

    base = _initBundleMappedUserBase;
    size = _initBundleMappedSize;

    return true;
  }

  UInt32 SpawnInitBundleTask(CString name) {
    if (_initBundleMappedSize == 0 || _initBundleMappedBase == 0) {
      return 0;
    }

    const BundleEntry* entry = FindEntryByName(name);

    if (!entry) {
      return 0;
    }

    if (entry->offset + entry->size > _initBundleMappedSize) {
      return 0;
    }

    constexpr UInt32 pageSize = 4096;
    const UInt8* bundleBase
      = reinterpret_cast<const UInt8*>(_initBundleMappedBase);
    const UInt8* payload = bundleBase + entry->offset;
    UInt32 size = entry->size;

    if (size < sizeof(UInt32)) {
      return 0;
    }

    UInt32 entryOffset = *reinterpret_cast<const UInt32*>(payload);

    if (entryOffset >= size) {
      return 0;
    }

    UInt32 addressSpace = Memory::CreateAddressSpace();

    if (addressSpace == 0) {
      return 0;
    }

    UInt32 pages = AlignUp(size, pageSize) / pageSize;

    for (UInt32 i = 0; i < pages; ++i) {
      void* phys = Memory::AllocatePage(true);
      UInt32 vaddr = _userProgramBase + i * pageSize;

      Memory::MapPageInAddressSpace(
        addressSpace,
        vaddr,
        reinterpret_cast<UInt32>(phys),
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

    UInt32 stackBytes = AlignUp(_userStackSize, pageSize);
    UInt32 stackBase = _userStackTop - stackBytes;
    UInt32 stackPages = stackBytes / pageSize;

    for (UInt32 i = 0; i < stackPages; ++i) {
      void* phys = Memory::AllocatePage(true);
      UInt32 vaddr = stackBase + i * pageSize;

      Memory::MapPageInAddressSpace(
        addressSpace,
        vaddr,
        reinterpret_cast<UInt32>(phys),
        true,
        true,
        false
      );
    }

    Task::ControlBlock* task = Task::CreateUser(
      _userProgramBase + entryOffset,
      _userStackTop,
      addressSpace
    );

    if (!task) {
      Memory::DestroyAddressSpace(addressSpace);

      return 0;
    }

    return task->id;
  }

  void Panic(
    String message,
    String file,
    UInt32 line,
    String function
  ) {
    Handlers::PanicHandler::Handle(
      message,
      file,
      line,
      function
    );
  }
}
