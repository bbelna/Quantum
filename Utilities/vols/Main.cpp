/**
 * @file Utilities/vols/Main.cpp
 * @brief Main entry point for the volumes command.
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

using namespace Quantum::Core;
using namespace Quantum::Clients;
using namespace Quantum::Servers::FileSystem::ABI;
using namespace Quantum::Streaming;

/**
 * @brief Formats a byte count as KB into @p buffer. Writes `"--"` when
 *        @p bytes is zero.
 * @param bytes The byte count to format.
 * @param buffer Destination buffer.
 * @param bufferSize Size of @p buffer.
 */
void FormatKB(UInt32 bytes, char* buffer, Size bufferSize) {
  if (bytes == 0) {
    CString::Copy("--", buffer, bufferSize);
  } else {
    CString::Format(buffer, bufferSize, "%u KB", bytes / 1024);
  }
}

/**
 * @brief Main entry point for the volumes utility.
 * @param argumentCount Number of arguments.
 * @param arguments Argument vector. `[1]` is the stream descriptor.
 * @return Exit code (`0` on success, `1` on failure).
 *
 * Lists all mounted volumes with their device IDs, labels, used space,
 * and total capacity.
 */
int Main(int argumentCount, char** arguments, char** environment) {
  if (!StandardOut.IsValid()) return 1;

  FileSystemClient fileSystem;
  FileSystemVolumeInfo volumes[16];
  Int32 count = fileSystem.ListVolumes(volumes, 16);

  if (count <= 0) {
    StandardOut.Write("No volumes mounted\n");

    return 0;
  }

  // find the longest value in each column
  constexpr Size maxColumnWidth = 20;

  Size volumeColumnWidth = CString::Length("Volume");
  Size labelColumnWidth = CString::Length("Label");
  Size usedColumnWidth = CString::Length("Used");
  Size totalColumnWidth = CString::Length("Total");

  for (Int32 index = 0; index < count; ++index) {
    Size length = CString::Length(volumes[index].VolumeID);

    if (length > volumeColumnWidth) volumeColumnWidth = length;

    length = CString::Length(volumes[index].Label);

    if (length > labelColumnWidth) labelColumnWidth = length;

    char tempBuffer[16];

    FormatKB(volumes[index].UsedBytes, tempBuffer, sizeof(tempBuffer));

    length = CString::Length(tempBuffer);

    if (length > usedColumnWidth) usedColumnWidth = length;

    FormatKB(volumes[index].TotalBytes, tempBuffer, sizeof(tempBuffer));

    length = CString::Length(tempBuffer);

    if (length > totalColumnWidth) totalColumnWidth = length;
  }

  // cap each column
  if (volumeColumnWidth > maxColumnWidth) volumeColumnWidth = maxColumnWidth;
  if (labelColumnWidth > maxColumnWidth) labelColumnWidth = maxColumnWidth;
  if (usedColumnWidth > maxColumnWidth) usedColumnWidth = maxColumnWidth;
  if (totalColumnWidth > maxColumnWidth) totalColumnWidth = maxColumnWidth;

  // print column headers
  char headerVolume[maxColumnWidth + 1];
  char headerLabel[maxColumnWidth + 1];
  char headerUsed[maxColumnWidth + 1];
  char headerTotal[maxColumnWidth + 1];

  CString::PadRight(
    "Volume",
    headerVolume,
    sizeof(headerVolume),
    volumeColumnWidth
  );
  CString::PadRight(
    "Label",
    headerLabel,
    sizeof(headerLabel),
    labelColumnWidth
  );
  CString::PadRight(
    "Used",
    headerUsed,
    sizeof(headerUsed),
    usedColumnWidth
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
    headerVolume,
    headerLabel,
    headerUsed,
    headerTotal
  );

  StandardOut.Write(headerLine);

  // print each volume
  for (Int32 index = 0; index < count; ++index) {
    char volumeColumn[maxColumnWidth + 1];
    char labelColumn[maxColumnWidth + 1];
    char usedColumn[maxColumnWidth + 1];
    char totalColumn[maxColumnWidth + 1];

    CString::PadRight(
      volumes[index].VolumeID,
      volumeColumn,
      sizeof(volumeColumn),
      volumeColumnWidth
    );
    CString::PadRight(
      volumes[index].Label,
      labelColumn,
      sizeof(labelColumn),
      labelColumnWidth
    );

    char usedString[16];
    char totalString[16];

    FormatKB(
      volumes[index].UsedBytes,
      usedString,
      sizeof(usedString)
    );
    FormatKB(
      volumes[index].TotalBytes,
      totalString,
      sizeof(totalString)
    );

    CString::PadRight(
      usedString,
      usedColumn,
      sizeof(usedColumn),
      usedColumnWidth
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
      volumeColumn,
      labelColumn,
      usedColumn,
      totalColumn
    );

    StandardOut.Write(line);
  }

  return 0;
}
