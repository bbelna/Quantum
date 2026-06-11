/**
 * @file Servers/Device/DeviceManager.hpp
 * @brief Declares the device manager class.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <DeviceServerTypes.hpp>

namespace Quantum::Servers::Device {
  /**
   * @brief Maximum number of device categories the manager can hold.
   */
  inline constexpr Size MaxCategories = 64;

  /**
   * @brief The device manager responsible for managing devices.
   */
  class DeviceManager {
    public:
      DeviceManager(
        KernelClient& kernel,
        ServerLog& log
      ) : _kernel(kernel), _log(log) {}

      /**
       * @brief Detects devices on the platform and populates the category
       *        lists.
       */
      void Discover();

      /**
       * @brief Gets all devices matching the specified category.
       * @param categoryID The category to filter by.
       * @return A PointerList of matching devices. The caller is responsible
       *         for freeing the underlying array with `delete[]`.
       */
      PointerList<Device> GetInCategory(DeviceCategoryID categoryID);

      /**
       * @brief Adds a new device to the appropriate category.
       * @param device The device to add. Its `CategoryID` determines which
       *        category it is placed in.
       * @return `true` if the device was added successfully, or `false` if the
       *         category is full.
       */
      bool Add(const Device& device);

    private:
      KernelClient& _kernel;

      ServerLog& _log;

      /**
       * @brief Registered device categories.
       */
      DeviceCategory _categories[MaxCategories] = {};

      /**
       * @brief Number of categories currently in use.
       */
      Size _categoryCount = 0;

      /**
       * @brief Finds or creates a category entry for the given ID.
       * @param categoryID The category ID to look up.
       * @return Pointer to the category, or `nullptr` if the category table is
       *         full and the ID was not found.
       */
      DeviceCategory* _findOrCreateCategory(DeviceCategoryID categoryID);
  };
}
