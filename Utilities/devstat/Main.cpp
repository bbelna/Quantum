/**
 * @file Utilities/devstat/Main.cpp
 * @brief Main entry point for the devices command.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Memory.hpp>
#include <Quantum/Clients.hpp>
#include <Quantum/HAL.hpp>
#include <Quantum/Streaming.hpp>

using namespace Quantum::Core;
using namespace Quantum::Clients;
using namespace Quantum::HAL;
using namespace Quantum::Streaming;
using namespace Quantum::Structures::Lists;

/**
 * @brief Converts a @ref DeviceState to a C-string.
 * @param state The @ref DeviceState to convert.
 * @return The provided @ref DeviceState as a C-string.
 */
const char* ToString(DeviceState state) {
  switch (state) {
    case DeviceState::Discovered: return "Discovered";
    case DeviceState::Bound: return "Bound";
    case DeviceState::Active: return "Active";
    case DeviceState::Disabled: return "Disabled";
    case DeviceState::Error: return "Error";
    default: return "Unknown";
  }
}

/**
 * @brief Formats a device's bus address into a human-readable string.
 * @param buffer Destination buffer.
 * @param bufferSize Size of @p buffer.
 * @param device The device whose bus address to format.
 */
void FormatBusAddress(
  char* buffer,
  Size bufferSize,
  const Device& device
) {
  if (device.Bus == DeviceBus::PCI) {
    PCIAddress pciAddress = GetPCIAddress(device);

    CString::Format(
      buffer,
      bufferSize,
      "PCI %u:%u.%u",
      pciAddress.Bus,
      pciAddress.Slot,
      pciAddress.Function
    );
  } else if (device.Bus == DeviceBus::ISA) {
    ISAAddress isaAddress = GetISAAddress(device);

    CString::Format(
      buffer,
      bufferSize,
      "ISA 0x%x:%u",
      isaAddress.IOBase,
      isaAddress.IRQ
    );
  } else if (device.Bus == DeviceBus::Virtual) {
    CString::Copy("Virtual", buffer, bufferSize);
  } else {
    CString::Copy("--", buffer, bufferSize);
  }
}

/**
 * @brief List of all device categories to query.
 */
constexpr DeviceCategoryType AllCategories[] = {
  DeviceCategoryType::CPU,
  DeviceCategoryType::Graphics,
  DeviceCategoryType::Input,
  DeviceCategoryType::Storage,
  DeviceCategoryType::Network,
  DeviceCategoryType::Audio,
  DeviceCategoryType::Bus,
  DeviceCategoryType::System,
  DeviceCategoryType::RTC,
  DeviceCategoryType::Chipset,
  DeviceCategoryType::DMA,
};

/**
 * @brief Main entry point for the devices command (`devices`).
 * @param argumentCount Number of arguments.
 * @param arguments Argument vector. `[1]` is the stream descriptor.
 * @return Exit code (`0` on success, `1` on failure).
 *
 * Lists all discovered hardware devices with their names, categories,
 * states, and bus addresses.
 */
int Main(int argumentCount, char** arguments, char** environment) {
  if (!StandardOut.IsValid()) return 1;

  // collect devices from all categories
  constexpr Size maxDevices = 64;
  UIntPtr devicesAddress = AllocateBlock(maxDevices * sizeof(Device));

  if (devicesAddress == 0) {
    StandardOut.Write("Out of memory\n");

    return 1;
  }

  DeviceClient deviceClient;
  Device* devices = reinterpret_cast<Device*>(devicesAddress);
  Size totalCount = 0;

  for (DeviceCategoryType category : AllCategories) {
    PointerList<Device> list = deviceClient.GetDevicesInCategory(
      ToDeviceCategoryID(category)
    );

    for (Size index = 0; index < list.GetCount(); ++index) {
      if (totalCount >= maxDevices) break;

      devices[totalCount++] = list[index];
    }
  }

  if (totalCount == 0) {
    FreeBlock(devicesAddress);

    StandardOut.Write("No devices found\n");

    return 0;
  }

  // find the longest value in each column
  constexpr Size maxColumnWidth = 20;
  const char* nameColumnLabel = "Name";
  const char* categoryColumnLabel = "Category";
  const char* stateColumnLabel = "State";
  const char* busColumnLabel = "Bus";
  Size nameColumnWidth = CString::Length(nameColumnLabel);
  Size categoryColumnWidth = CString::Length(categoryColumnLabel);
  Size stateColumnWidth = CString::Length(stateColumnLabel);
  Size busColumnWidth = CString::Length(busColumnLabel);

  for (Size index = 0; index < totalCount; ++index) {
    Size length = CString::Length(devices[index].Name);
    DeviceCategoryType category = static_cast<DeviceCategoryType>(
      devices[index].CategoryID
    );

    if (length > nameColumnWidth) {
      nameColumnWidth = length;
    }

    length = CString::Length(ToString(category));

    if (length > categoryColumnWidth) {
      categoryColumnWidth = length;
    }

    length = CString::Length(ToString(devices[index].State));

    if (length > stateColumnWidth) {
      stateColumnWidth = length;
    }

    char busString[20];

    FormatBusAddress(busString, sizeof(busString), devices[index]);

    length = CString::Length(busString);

    if (length > busColumnWidth) {
      busColumnWidth = length;
    }
  }

  // cap each column
  if (nameColumnWidth > maxColumnWidth) {
    nameColumnWidth = maxColumnWidth;
  }

  if (categoryColumnWidth > maxColumnWidth) {
    categoryColumnWidth = maxColumnWidth;
  }

  if (stateColumnWidth > maxColumnWidth) {
    stateColumnWidth = maxColumnWidth;
  }

  if (busColumnWidth > maxColumnWidth) {
    busColumnWidth = maxColumnWidth;
  }

  // print column headers
  char headerName[maxColumnWidth + 1];
  char headerCategory[maxColumnWidth + 1];
  char headerState[maxColumnWidth + 1];
  char headerBus[maxColumnWidth + 1];

  CString::PadRight(
    nameColumnLabel,
    headerName,
    sizeof(headerName),
    nameColumnWidth
  );
  CString::PadRight(
    categoryColumnLabel,
    headerCategory,
    sizeof(headerCategory),
    categoryColumnWidth
  );
  CString::PadRight(
    stateColumnLabel,
    headerState,
    sizeof(headerState),
    stateColumnWidth
  );
  CString::PadRight(
    busColumnLabel,
    headerBus,
    sizeof(headerBus),
    busColumnWidth
  );

  char headerLine[128];

  CString::Format(
    headerLine,
    sizeof(headerLine),
    "%s  %s  %s  %s\n",
    headerName,
    headerCategory,
    headerState,
    headerBus
  );

  StandardOut.Write(headerLine);

  // print each device
  for (Size index = 0; index < totalCount; ++index) {
    char nameColumn[maxColumnWidth + 1];
    char categoryColumn[maxColumnWidth + 1];
    char stateColumn[maxColumnWidth + 1];
    char busString[20];
    DeviceCategoryType category = static_cast<DeviceCategoryType>(
      devices[index].CategoryID
    );

    CString::PadRight(
      devices[index].Name,
      nameColumn,
      sizeof(nameColumn),
      nameColumnWidth
    );
    CString::PadRight(
      ToString(category),
      categoryColumn,
      sizeof(categoryColumn),
      categoryColumnWidth
    );
    CString::PadRight(
      ToString(devices[index].State),
      stateColumn,
      sizeof(stateColumn),
      stateColumnWidth
    );
    FormatBusAddress(busString, sizeof(busString), devices[index]);

    char busColumn[maxColumnWidth + 1];

    CString::PadRight(
      busString,
      busColumn,
      sizeof(busColumn),
      busColumnWidth
    );

    char line[128];

    CString::Format(
      line,
      sizeof(line),
      "%s  %s  %s  %s\n",
      nameColumn,
      categoryColumn,
      stateColumn,
      busColumn
    );

    StandardOut.Write(line);
  }

  FreeBlock(devicesAddress);

  return 0;
}
