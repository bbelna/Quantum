/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Coordinator/Coordinator.cpp
 * System coordinator implementation.
 */

#include <ABI/InitBundle.hpp>
#include <Console.hpp>
#include <Task.hpp>

#include "Coordinator.hpp"

namespace Quantum::System::Coordinator {
  namespace {
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

    UInt32 EntryNameLength(const BundleEntry& entry) {
      UInt32 len = 0;

      while (len < 32 && entry.name[len] != '\0') {
        ++len;
      }

      return len;
    }
  }

  void Main() {
    Console::WriteLine("Coordinator");

    Quantum::ABI::InitBundle::Info info{};
    bool ok = Quantum::ABI::InitBundle::GetInfo(info);

    if (!ok || info.base == 0 || info.size == 0) {
      Console::WriteLine("INIT.BND not available");
      Task::Exit(1);
    }

    const UInt8* base = reinterpret_cast<const UInt8*>(info.base);

    if (info.size < sizeof(BundleHeader)) {
      Console::WriteLine("INIT.BND too small");
      Task::Exit(1);
    }

    const BundleHeader* header = reinterpret_cast<const BundleHeader*>(base);

    if (!HasMagic(*header)) {
      Console::WriteLine("INIT.BND bad magic");
      Task::Exit(1);
    }

    UInt32 tableOffset = header->tableOffset;
    UInt32 entryCount = header->entryCount;
    UInt32 tableBytes = entryCount * static_cast<UInt32>(sizeof(BundleEntry));

    if (tableOffset + tableBytes > info.size) {
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
        Console::Write(entry.name);
        Console::WriteLine("");
      } else {
        Console::WriteLine("(unnamed)");
      }
    }

    Console::WriteLine("INIT.BND parsed");
    Task::Exit(0);
  }
}
