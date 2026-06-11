/**
 * @file Utilities/ps/Main.cpp
 * @brief Main entry point for the ps command.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Memory.hpp>
#include <Quantum/Clients.hpp>
#include <Quantum/Kernel.hpp>
#include <Quantum/Streaming.hpp>

using namespace Quantum::Core;
using namespace Quantum;
using namespace Quantum::Clients;
using namespace Quantum::Kernel::ABI;
using namespace Quantum::Streaming;

/**
 * @brief Main entry point for the ps utility.
 * @param argumentCount Number of arguments.
 * @param arguments Argument vector. `[1]` is the stream descriptor.
 * @return Exit code (`0` on success, `1` on failure).
 *
 * Lists all running processes with their PIDs, names, and memory usage,
 * followed by a system memory summary.
 */
int Main(int argumentCount, char** arguments, char** environment) {
  if (!StandardOut.IsValid()) return 1;

  constexpr UInt32 maxProcesses = 128;
  constexpr Size maxColumnWidth = 20;

  KernelClient kernel;
  UIntPtr entriesAddress = AllocateBlock(maxProcesses * sizeof(ProcessInfo));

  if (entriesAddress == 0) {
    StandardOut.Write("Out of memory\n");

    return 1;
  }

  ProcessInfo* processes = reinterpret_cast<ProcessInfo*>(entriesAddress);
  UInt32 count = kernel.GetProcessList(processes, maxProcesses);

  if (count == 0) {
    FreeBlock(entriesAddress);

    StandardOut.Write("No processes found\n");

    return 0;
  }

  // fetch kernel memory info once for PID 0
  KernelMemoryInfo kernelMemoryInfo = {};
  bool hasKernelMemoryInfo = kernel.GetMemoryInfo(&kernelMemoryInfo);

  // find the longest value in each column
  Size pidColumnWidth = CString::Length("PID");
  Size nameColumnWidth = CString::Length("Name");
  Size heapColumnWidth = CString::Length("Heap");
  Size totalColumnWidth = CString::Length("Total");

  for (UInt32 index = 0; index < count; ++index) {
    char tempBuffer[16];

    CString::Format(
      tempBuffer,
      sizeof(tempBuffer),
      "%u",
      processes[index].ID
    );

    Size length = CString::Length(tempBuffer);

    if (length > pidColumnWidth) pidColumnWidth = length;

    length = CString::Length(processes[index].Name);

    if (length > nameColumnWidth) nameColumnWidth = length;

    UInt32 heapKB = processes[index].HeapPageCount * 4;
    UInt32 totalKB = processes[index].TotalPageCount * 4;

    if (processes[index].ID == 0 && hasKernelMemoryInfo) {
      heapKB = kernelMemoryInfo.KernelHeapBytes / 1024;
      totalKB
        = (
          kernelMemoryInfo.KernelReservedBytes +
          kernelMemoryInfo.InitialImageBytes +
          kernelMemoryInfo.KernelHeapBytes
        ) / 1024;
    }

    if (heapKB == 0) {
      CString::Copy("--", tempBuffer, sizeof(tempBuffer));
    } else {
      CString::Format(tempBuffer, sizeof(tempBuffer), "%u KB", heapKB);
    }

    length = CString::Length(tempBuffer);

    if (length > heapColumnWidth) heapColumnWidth = length;

    if (totalKB == 0) {
      CString::Copy("--", tempBuffer, sizeof(tempBuffer));
    } else {
      CString::Format(tempBuffer, sizeof(tempBuffer), "%u KB", totalKB);
    }

    length = CString::Length(tempBuffer);

    if (length > totalColumnWidth) totalColumnWidth = length;
  }

  // cap each column
  if (pidColumnWidth > maxColumnWidth) pidColumnWidth = maxColumnWidth;
  if (nameColumnWidth > maxColumnWidth) nameColumnWidth = maxColumnWidth;
  if (heapColumnWidth > maxColumnWidth) heapColumnWidth = maxColumnWidth;
  if (totalColumnWidth > maxColumnWidth) totalColumnWidth = maxColumnWidth;

  // print column headers
  char headerPID[maxColumnWidth + 1];
  char headerName[maxColumnWidth + 1];
  char headerHeap[maxColumnWidth + 1];
  char headerTotal[maxColumnWidth + 1];

  CString::PadRight(
    "PID",
    headerPID,
    sizeof(headerPID),
    pidColumnWidth
  );
  CString::PadRight(
    "Name",
    headerName,
    sizeof(headerName),
    nameColumnWidth
  );
  CString::PadRight(
    "Heap",
    headerHeap,
    sizeof(headerHeap),
    heapColumnWidth
  );
  CString::PadRight(
    "Total",
    headerTotal,
    sizeof(headerTotal),
    totalColumnWidth
  );

  char headerLine[128];

  CString::Format(
    headerLine,
    sizeof(headerLine),
    "%s  %s  %s  %s\n",
    headerPID,
    headerName,
    headerHeap,
    headerTotal
  );

  StandardOut.Write(headerLine);

  // print each process
  for (UInt32 index = 0; index < count; ++index) {
    char pidString[16];

    CString::Format(pidString, sizeof(pidString), "%u", processes[index].ID);

    char pidColumn[maxColumnWidth + 1];

    CString::PadRight(
      pidString,
      pidColumn,
      sizeof(pidColumn),
      pidColumnWidth
    );

    char nameColumn[maxColumnWidth + 1];

    CString::PadRight(
      processes[index].Name,
      nameColumn,
      sizeof(nameColumn),
      nameColumnWidth
    );

    UInt32 heapKB = processes[index].HeapPageCount * 4;
    UInt32 totalKB = processes[index].TotalPageCount * 4;

    // PID 0 is the kernel -- substitute real memory from MemoryInfo
    if (processes[index].ID == 0 && hasKernelMemoryInfo) {
      heapKB = kernelMemoryInfo.KernelHeapBytes / 1024;
      totalKB
        = (
          kernelMemoryInfo.KernelReservedBytes +
          kernelMemoryInfo.InitialImageBytes +
          kernelMemoryInfo.KernelHeapBytes
        ) / 1024;
    }

    char heapString[16];
    char totalString[16];

    if (heapKB == 0) {
      CString::Copy("--", heapString, sizeof(heapString));
    } else {
      CString::Format(heapString, sizeof(heapString), "%u KB", heapKB);
    }

    if (totalKB == 0) {
      CString::Copy("--", totalString, sizeof(totalString));
    } else {
      CString::Format(totalString, sizeof(totalString), "%u KB", totalKB);
    }

    char heapColumn[maxColumnWidth + 1];
    char totalColumn[maxColumnWidth + 1];

    CString::PadRight(
      heapString,
      heapColumn,
      sizeof(heapColumn),
      heapColumnWidth
    );
    CString::PadRight(
      totalString,
      totalColumn,
      sizeof(totalColumn),
      totalColumnWidth
    );

    char line[128];

    CString::Format(
      line,
      sizeof(line),
      "%s  %s  %s  %s\n",
      pidColumn,
      nameColumn,
      heapColumn,
      totalColumn
    );

    StandardOut.Write(line);
  }

  FreeBlock(entriesAddress);

  return 0;
}
