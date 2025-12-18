/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Coordinator/Coordinator.cpp
 * System coordinator implementation.
 */

#include <Console.hpp>
#include <Task.hpp>
#include <ABI/InitBundle.hpp>
#include <ABI/Types/InitBundleInfo.hpp>

#include "Coordinator.hpp"

namespace Quantum::System::Coordinator {
  namespace {
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

    UInt32 EntryNameLength(const BundleEntry& entry) {
      UInt32 len = 0;

      while (len < 32 && entry.Name[len] != '\0') {
        ++len;
      }

      return len;
    }
  }

  void Main() {
    Console::WriteLine("Coordinator");

    Quantum::ABI::Types::InitBundleInfo info{};
    bool ok = Quantum::ABI::GetInitBundleInfo(info);

    if (!ok || info.Base == 0 || info.Size == 0) {
      Console::WriteLine("INIT.BND not available");
      Task::Exit(1);
    }

    const UInt8* base = reinterpret_cast<const UInt8*>(info.Base);

    if (info.Size < sizeof(BundleHeader)) {
      Console::WriteLine("INIT.BND too small");
      Task::Exit(1);
    }

    const BundleHeader* header = reinterpret_cast<const BundleHeader*>(base);

    if (!HasMagic(*header)) {
      Console::WriteLine("INIT.BND bad magic");
      Task::Exit(1);
    }

    UInt32 tableOffset = header->TableOffset;
    UInt32 entryCount = header->EntryCount;
    UInt32 tableBytes = entryCount * static_cast<UInt32>(sizeof(BundleEntry));

    if (tableOffset + tableBytes > info.Size) {
      Console::WriteLine("INIT.BND table out of range");
      Task::Exit(1);
    }

    const BundleEntry* entries
      = reinterpret_cast<const BundleEntry*>(base + tableOffset);

    for (UInt32 i = 0; i < entryCount; ++i) {
      const BundleEntry& entry = entries[i];
      UInt32 nameLen = EntryNameLength(entry);

      Console::WriteLine("INIT entry:");

      if (nameLen > 0) {
        Console::Write(entry.Name);
        Console::WriteLine("");
      } else {
        Console::WriteLine("(unnamed)");
      }
    }

    Console::WriteLine("INIT.BND parsed");
    Task::Exit(0);
  }
}
