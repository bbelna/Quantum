/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Kernel.cpp
 * Core kernel implementation.
 */

#include <CPU.hpp>
#include <Interrupts.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Memory.hpp>
#include <Task.hpp>
#include <Testing.hpp>
#include <Arch/IA32/Memory.hpp>
#include <Arch/IA32/UserMode.hpp>
#include <Handlers/PanicHandler.hpp>
#include <Handlers/SystemCallHandler.hpp>
#include <Helpers/CStringHelper.hpp>
#include <Helpers/DebugHelper.hpp>
#include <Types/Primitives.hpp>
#include <Types/Boot/BootInfo.hpp>
#include <Types/Logging/LogLevel.hpp>

namespace Quantum::System::Kernel {
  using Helpers::CStringHelper;
  using Types::Logging::LogLevel;

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

    void MapInitBundle(UInt32 bootInfoPhysicalAddress) {
      auto* bootInfo = reinterpret_cast<Types::Boot::BootInfo*>(
        bootInfoPhysicalAddress
      );

      if (!bootInfo || bootInfo->InitBundleSize == 0) {
        return;
      }

      UInt32 size = bootInfo->InitBundleSize;
      UInt32 base = bootInfo->InitBundlePhysical;
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

        Arch::IA32::Memory::MapPage(virt, phys, false, false, false);
        Arch::IA32::Memory::MapPage(
          _initBundleUserBase + i * 4096,
          phys,
          false,
          true,
          false
        );
      }

      _initBundleMappedBase = _initBundleVirtualBase;
      _initBundleMappedSize = size;
      _initBundleMappedUserBase = _initBundleUserBase;
    }

    struct BundleHeader {
      char Magic[8];
      UInt16 Version;
      UInt16 EntryCount;
      UInt32 TableOffset;
      UInt8 Reserved[8];
    };

    struct BundleEntry {
      char Name[32];
      UInt8 Type;
      UInt8 Flags;
      UInt8 Reserved[2];
      UInt32 Offset;
      UInt32 Size;
      UInt32 Checksum;
    };

    bool HasMagic(const BundleHeader& header) {
      const char expected[8] = { 'I','N','I','T','B','N','D','\0' };

      for (UInt32 i = 0; i < 8; ++i) {
        if (header.Magic[i] != expected[i]) {
          return false;
        }
      }

      return true;
    }

    const BundleEntry* FindCoordinatorEntry() {
      if (_initBundleMappedSize < sizeof(BundleHeader)) {
        return nullptr;
      }

      const UInt8* base = reinterpret_cast<const UInt8*>(_initBundleMappedBase);
      const BundleHeader* header
        = reinterpret_cast<const BundleHeader*>(base);

      if (!HasMagic(*header)) {
        UInt32 magic0 = *reinterpret_cast<const UInt32*>(header->Magic);
        UInt32 magic1 = *reinterpret_cast<const UInt32*>(header->Magic + 4);
        Logger::WriteFormatted(
          LogLevel::Debug,
          "INIT.BND bad magic: magic0=0x%x magic1=0x%x ver=%u entries=%u table=0x%x",
          magic0,
          magic1,
          static_cast<UInt32>(header->Version),
          static_cast<UInt32>(header->EntryCount),
          header->TableOffset
        );
        char magic[9];
        for (UInt32 i = 0; i < 8; ++i) {
          magic[i] = header->Magic[i];
        }
        magic[8] = '\0';

        Logger::WriteFormatted(
          LogLevel::Warning,
          "INIT.BND magic mismatch (%s)",
          magic
        );
        return nullptr;
      }

      UInt32 tableOffset = header->TableOffset;
      UInt32 entryCount = header->EntryCount;
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

      const BundleEntry* entries
        = reinterpret_cast<const BundleEntry*>(base + tableOffset);

      for (UInt32 i = 0; i < entryCount; ++i) {
        if (entries[i].Type == 1) {
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

      if (entry->Offset + entry->Size > _initBundleMappedSize) {
        Logger::Write(LogLevel::Warning, "Coordinator entry out of range");
        Task::Exit();
      }

      const UInt8* bundleBase
        = reinterpret_cast<const UInt8*>(_initBundleMappedBase);
      const UInt8* payload = bundleBase + entry->Offset;
      UInt32 size = entry->Size;
      UInt32 pages = AlignUp(size, 4096) / 4096;

      for (UInt32 i = 0; i < pages; ++i) {
        void* phys = Memory::AllocatePage(true);
        UInt32 vaddr = _userProgramBase + i * 4096;

        Arch::IA32::Memory::MapPage(
          vaddr,
          reinterpret_cast<UInt32>(phys),
          true,
          true,
          false
        );
      }

      UInt8* dest = reinterpret_cast<UInt8*>(_userProgramBase);

      for (UInt32 i = 0; i < size; ++i) {
        dest[i] = payload[i];
      }

      UInt32 entryOffset = *reinterpret_cast<UInt32*>(dest);

      Arch::IA32::UserMode::MapUserStack(_userStackTop, _userStackSize);
      Arch::IA32::UserMode::Enter(
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
    Memory::Initialize(bootInfoPhysicalAddress);
    Interrupts::Initialize();
    Task::Initialize();
    Task::EnablePreemption();
    MapInitBundle(bootInfoPhysicalAddress);

    Task::Create(LaunchCoordinatorFromBundle, 4096);

    #ifdef KERNEL_TESTS
    // spawn test runner task and start scheduling
    Task::Create(KernelTestRunner, 4096);
    #endif

    // enter scheduler; if no tests are queued we fall back to idle
    Task::Yield();
  }

  InterruptContext* HandleSystemCall(InterruptContext& context) {
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
