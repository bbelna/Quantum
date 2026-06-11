/**
 * @file Servers/Device/DeviceManager.cpp
 * @brief Implements @ref @QDvSrv::DeviceManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "DeviceManager.hpp"

namespace Quantum::Servers::Device {
  void DeviceManager::Discover() {
    PointerList<Device> devices = _kernel.GetDevices();

    for (Size i = 0; i < devices.GetCount(); i++) {
      Device& device = devices[i];

      // skip devices that the kernel explicitly disabled (e.g. VESA when
      // a hardware-accelerated driver like S3 ViRGE is active)
      if (device.State == DeviceState::Disabled) continue;

      _log.Write(
        LogLevel::Info,
        "Discovered %s (%s)",
        device.Name,
        device.DisplayName
      );

      Add(device);
    }
  }

  PointerList<Device> DeviceManager::GetInCategory(
    DeviceCategoryID categoryID
  ) {
    for (
      Size categoryIndex = 0;
      categoryIndex < _categoryCount;
      categoryIndex++
    ) {
      if (_categories[categoryIndex].ID != categoryID) continue;

      DeviceCategory& category = _categories[categoryIndex];

      if (category.DeviceCount == 0) {
        return PointerList<Device>();
      } else {
        Device* output = new Device[category.DeviceCount];

        for (
          Size deviceIndex = 0;
          deviceIndex < category.DeviceCount;
          deviceIndex++
        ) {
          output[deviceIndex] = *category.Devices[deviceIndex];
        }

        return PointerList<Device>(output, category.DeviceCount);
      }
    }

    return PointerList<Device>();
  }

  bool DeviceManager::Add(const Device& device) {
    DeviceCategory* category = _findOrCreateCategory(device.CategoryID);

    if (
      !category ||
      category->DeviceCount >= DeviceCategoryMaxDevices
    ) {
      return false;
    } else {
      Device* stored = new Device(device);

      category->Devices[category->DeviceCount++] = stored;

      return true;
    }
  }

  DeviceCategory* DeviceManager::_findOrCreateCategory(
    DeviceCategoryID categoryID
  ) {
    for (Size i = 0; i < _categoryCount; i++) {
      if (_categories[i].ID == categoryID) return &_categories[i];
    }

    if (_categoryCount >= MaxCategories) {
      return nullptr;
    } else {
      DeviceCategory& category = _categories[_categoryCount++];

      category.ID = categoryID;
      category.ParentID = ToDeviceCategoryID(DeviceCategoryType::None);
      category.DeviceCount = 0;

      DeviceCategoryType categoryType =
        static_cast<DeviceCategoryType>(categoryID);
      const char* name = ToString(categoryType);

      CString::Copy(
        name,
        category.Name,
        DeviceCategoryNameMaxLength
      );
      CString::Copy(
        name,
        category.DisplayName,
        DeviceCategoryDisplayNameMaxLength
      );

      return &category;
    }
  }
}
