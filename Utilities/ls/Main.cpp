/**
 * @file Utilities/ls/Main.cpp
 * @brief Main entry point for the ls command.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Memory.hpp>
#include <Quantum/Clients.hpp>
#include <Quantum/Servers.hpp>
#include <Quantum/Streaming.hpp>
#include <Quantum/Threading.hpp>

using namespace Quantum::Core;
using namespace Quantum::Clients;
using namespace Quantum::Servers::FileSystem::ABI;
using namespace Quantum::Streaming;
using namespace Quantum::Threading;

/**
 * @brief Main entry point for the ls utility.
 * @param argumentCount Number of arguments.
 * @param arguments Argument vector. `[0]` is the command name, `[1]` is
 *        the stream descriptor, `[2]` is an optional target directory.
 * @return Exit code (`0` on success, `1` on failure).
 *
 * Lists the contents of the current working directory (or a specified
 * target) and writes the result to the StandardOut.
 */
int Main(int argumentCount, char** arguments) {
  if (!StandardOut.IsValid()) return 1;

  constexpr Size maxColumnWidth = 20;
  constexpr UInt32 maxEntries = 256;

  // determine which directory to list
  char directoryPath[FileSystemMaxPathLength];

  if (argumentCount >= 2) {
    // explicit path argument: resolve relative to cwd
    char workingDirectory[MaxWorkingDirectoryLength];
    bool hasWorkingDirectory = Process::GetWorkingDirectory(
      Process::GetCurrentProcessID(),
      workingDirectory,
      sizeof(workingDirectory)
    );
    const char* target = arguments[1];

    // if the target starts with a volume label (contains no leading
    // slash and has content), treat it as absolute
    if (hasWorkingDirectory && target[0] != '/') {
      CString::Format(
        directoryPath,
        sizeof(directoryPath),
        "%s/%s",
        workingDirectory,
        target
      );
    } else {
      CString::Copy(target, directoryPath, sizeof(directoryPath));
    }
  } else {
    // no argument: list the working directory
    bool hasWorkingDirectory = Process::GetWorkingDirectory(
      Process::GetCurrentProcessID(),
      directoryPath,
      sizeof(directoryPath)
    );

    if (!hasWorkingDirectory || directoryPath[0] == '\0') {
      StandardOut.Write("No working directory set\n");

      return 1;
    }
  }

  FileSystemClient fileSystemClient;
  UIntPtr directoryAddress = AllocateBlock(maxEntries * sizeof(FileSystemDirectory));

  if (directoryAddress == 0) {
    StandardOut.Write("Out of memory\n");

    return 1;
  }

  FileSystemDirectory* directory = reinterpret_cast<FileSystemDirectory*>(
    directoryAddress
  );
  Int32 count = fileSystemClient.ReadDirectory(
    directoryPath,
    directory,
    maxEntries
  );

  if (count < 0) {
    FreeBlock(directoryAddress);

    StandardOut.Write("Failed to read directory\n");

    return 1;
  }

  if (count == 0) {
    FreeBlock(directoryAddress);

    StandardOut.Write("(empty)\n");

    return 0;
  }

  // sort: directories first (a-z), then files (a-z)
  Sort::InsertionSort(
    directory,
    static_cast<Size>(count),
    [](
      const FileSystemDirectory& left,
      const FileSystemDirectory& right
    ) -> bool {
      bool leftIsDirectory = left.Type == FileSystemEntryType::Directory;
      bool rightIsDirectory = right.Type == FileSystemEntryType::Directory;

      return leftIsDirectory != rightIsDirectory
        ? leftIsDirectory
        : CString::Compare(left.Name, right.Name) < 0;
    }
  );

  // pre-compute entry sizes so we can size columns in one pass
  UIntPtr sizesAddress = AllocateBlock(
    static_cast<Size>(count) * sizeof(UInt32)
  );

  if (sizesAddress == 0) {
    FreeBlock(directoryAddress);

    StandardOut.Write("Out of memory\n");

    return 1;
  }

  UInt32* sizes = reinterpret_cast<UInt32*>(sizesAddress);
  Size nameColumnWidth = CString::Length("Name");
  Size sizeColumnWidth = CString::Length("Size");

  for (Int32 index = 0; index < count; ++index) {
    Size nameLength = CString::Length(directory[index].Name);

    if (directory[index].Type == FileSystemEntryType::Directory) {
      nameLength += 1;
    }

    if (nameLength > nameColumnWidth) nameColumnWidth = nameLength;

    UInt32 entrySize = directory[index].Size;

    if (directory[index].Type == FileSystemEntryType::Directory) {
      char childPath[FileSystemMaxPathLength];

      CString::Format(
        childPath,
        sizeof(childPath),
        "%s/%s",
        directoryPath,
        directory[index].Name
      );

      entrySize = fileSystemClient.GetDirectorySize(childPath);
    }

    sizes[index] = entrySize;

    char sizeString[16];

    if (entrySize == 0) {
      CString::Copy("--", sizeString, sizeof(sizeString));
    } else {
      UInt32 entrySizeInKB = entrySize / 1024;

      CString::Format(
        sizeString,
        sizeof(sizeString),
        entrySizeInKB > 0
          ? "%u KB"
          : "%u B",
        entrySizeInKB > 0
          ? entrySizeInKB
          : entrySize
      );
    }

    Size sizeLength = CString::Length(sizeString);

    if (sizeLength > sizeColumnWidth) sizeColumnWidth = sizeLength;
  }

  // cap each column
  if (nameColumnWidth > maxColumnWidth) nameColumnWidth = maxColumnWidth;
  if (sizeColumnWidth > maxColumnWidth) sizeColumnWidth = maxColumnWidth;

  // print column headers
  char headerName[maxColumnWidth + 1];
  char headerSize[maxColumnWidth + 1];

  CString::PadRight(
    "Name",
    headerName,
    sizeof(headerName),
    nameColumnWidth
  );
  CString::PadRight(
    "Size",
    headerSize,
    sizeof(headerSize),
    sizeColumnWidth
  );

  char headerLine[FileSystemMaxPathLength + 32];

  CString::Format(
    headerLine,
    sizeof(headerLine),
    "%s  %s\n",
    headerName,
    headerSize
  );
  StandardOut.Write(headerLine);

  // print each entry
  for (Int32 index = 0; index < count; ++index) {
    char displayName[FileSystemMaxPathLength];

    if (directory[index].Type == FileSystemEntryType::Directory) {
      CString::Format(
        displayName,
        sizeof(displayName),
        "%s/",
        directory[index].Name
      );
    } else {
      CString::Copy(
        directory[index].Name,
        displayName,
        sizeof(displayName)
      );
    }

    char nameColumn[maxColumnWidth + 1];

    CString::PadRight(
      displayName,
      nameColumn,
      sizeof(nameColumn),
      nameColumnWidth
    );

    char sizeString[16];

    if (sizes[index] == 0) {
      CString::Copy("--", sizeString, sizeof(sizeString));
    } else {
      UInt32 entrySizeInKB = sizes[index] / 1024;

      CString::Format(
        sizeString,
        sizeof(sizeString),
        entrySizeInKB > 0
          ? "%u KB"
          : "%u B",
        entrySizeInKB > 0
          ? entrySizeInKB
          : sizes[index]
      );
    }

    char sizeColumn[maxColumnWidth + 1];

    CString::PadRight(
      sizeString,
      sizeColumn,
      sizeof(sizeColumn),
      sizeColumnWidth
    );

    char line[FileSystemMaxPathLength + 32];

    CString::Format(
      line,
      sizeof(line),
      "%s  %s\n",
      nameColumn,
      sizeColumn
    );
    StandardOut.Write(line);
  }

  FreeBlock(sizesAddress);
  FreeBlock(directoryAddress);

  return 0;
}
