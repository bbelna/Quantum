/**
 * @file Utilities/cat/Main.cpp
 * @brief Main entry point for the cat command.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
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
 * @brief Main entry point for the cat command (`cat`).
 * @param argumentCount Number of arguments.
 * @param arguments Argument vector. `[1]` is the file to display.
 * @return Exit code (`0` on success, `1` on failure).
 *
 * Reads the contents of a file and writes them to stdout.
 */
int Main(int argumentCount, char** arguments, char** environment) {
  if (!StandardOut.IsValid()) return 1;

  if (argumentCount < 2) {
    StandardOut.Write("Usage: cat <file>\n");

    return 1;
  }

  // resolve the file path relative to the working directory
  char path[FileSystemClient::MaxPathLength];
  char workingDirectory[MaxWorkingDirectoryLength];
  bool hasWorkingDirectory = Process::GetWorkingDirectory(
    Process::GetCurrentProcessID(),
    workingDirectory,
    sizeof(workingDirectory)
  );
  const char* target = arguments[1];

  if (hasWorkingDirectory && target[0] != '/') {
    CString::Format(
      path,
      sizeof(path),
      "%s/%s",
      workingDirectory,
      target
    );
  } else {
    CString::Copy(target, path, sizeof(path));
  }

  FileSystemClient fileSystem;
  FileSystemFileStat fileStat = {};

  if (!fileSystem.Stat(path, &fileStat) || fileStat.Size == 0) {
    StandardOut.Write("File not found or empty\n");

    return 1;
  }

  FileHandle fileHandle = fileSystem.Open(
    path,
    static_cast<UInt32>(FileSystemOpenFlags::Read)
  );

  if (fileHandle == 0) {
    StandardOut.Write("Failed to open file\n");

    return 1;
  }

  UIntPtr bufferAddress = AllocateBlock(fileStat.Size + 1);

  if (bufferAddress == 0) {
    fileSystem.Close(fileHandle);
    StandardOut.Write("Out of memory\n");

    return 1;
  }

  char* data = reinterpret_cast<char*>(bufferAddress);
  Int32 bytesRead = fileSystem.Read(fileHandle, data, fileStat.Size, 0);

  fileSystem.Close(fileHandle);

  if (bytesRead > 0) {
    data[bytesRead] = '\0';

    StandardOut.Write(data);

    if (data[bytesRead - 1] != '\n') {
      StandardOut.Write("\n", 1);
    }
  }

  FreeBlock(bufferAddress);

  return 0;
}
