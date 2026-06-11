/**
 * @file Menus/MenuBuilder.cpp
 * @brief Implements @ref @QMenus::MenuBuilder.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Menus/MenuBuilder.hpp>

#include "MenuTypes.hpp"

namespace Quantum::Menus {
  MenuBuilder::~MenuBuilder() {
    Reset();
  }

  void MenuBuilder::Reset() {
    for (Size index = 0; index < _menuCount; ++index) {
      _menus[index].FreeItems();
    }

    delete[] _menus;

    _menus = nullptr;
    _menuCount = 0;
    _menuCapacity = 0;

    delete[] _suppressed;

    _suppressed = nullptr;
    _suppressedCount = 0;
    _suppressedCapacity = 0;
  }

  void MenuBuilder::AddToStandardMenu(
    StandardMenuID menu,
    const MenuItem& item
  ) {
    UInt8 handle = _findOrCreateStandardMenu(menu);

    if (handle != InvalidUInt8) {
      _menus[handle].AddItem(item);
    }
  }

  UInt8 MenuBuilder::CreateMenu(
    const char* title,
    UInt16 position
  ) {
    if (_growMenus()) {
      UInt8 handle = static_cast<UInt8>(_menuCount);
      Menu& menu = _menus[_menuCount];

      menu = Menu{};

      menu.ID = _nextMenuID++;

      CString::Copy(
        title,
        menu.Title,
        MaxMenuTitleLength
      );

      menu.Position = position;

      _menuCount++;

      return handle;
    } else {
      return InvalidUInt8;
    }
  }

  UInt8 MenuBuilder::CreateFocusMenu(const char* title) {
    UInt8 handle = CreateMenu(title, 1);

    if (handle != InvalidUInt8) {
      _menus[handle].IsFocusMenu = true;
    }

    return handle;
  }

  void MenuBuilder::AddItem(UInt8 menuHandle, const MenuItem& item) {
    if (menuHandle < _menuCount) {
      _menus[menuHandle].AddItem(item);
    }
  }

  void MenuBuilder::SuppressAction(ActionID action) {
    if (_growSuppressed()) {
      _suppressed[_suppressedCount] = action;
      _suppressedCount++;
    }
  }

  bool MenuBuilder::_growMenus() {
    if (_menuCount >= _menuCapacity) {
      Size newCapacity = (_menuCapacity == 0) ? 4 : _menuCapacity * 2;
      auto* newMenus = new Menu[newCapacity];

      for (Size index = 0; index < _menuCount; ++index) {
        newMenus[index] = _menus[index];

        // detach source so it doesn't double-free items
        _menus[index].Items = nullptr;
        _menus[index].ItemCount = 0;
        _menus[index].ItemCapacity = 0;
      }

      delete[] _menus;

      _menus = newMenus;
      _menuCapacity = newCapacity;
    }

    return true;
  }

  bool MenuBuilder::_growSuppressed() {
    if (_suppressedCount >= _suppressedCapacity) {
      Size newCapacity
        = _suppressedCapacity == 0
        ? 4
        : _suppressedCapacity * 2;
      ActionID* newArray = new ActionID[newCapacity];

      for (Size index = 0; index < _suppressedCount; ++index) {
        newArray[index] = _suppressed[index];
      }

      delete[] _suppressed;

      _suppressed = newArray;
      _suppressedCapacity = newCapacity;
    }

    return true;
  }

  UInt8 MenuBuilder::_findOrCreateStandardMenu(StandardMenuID menu) {
    MenuID id = 0;
    UInt16 position = 0;
    const char* title = "";

    switch (menu) {
      case StandardMenuID::System: {
        id = 1;
        position = 0;
        title = "\xC4";

        break;
      }

      case StandardMenuID::Edit: {
        id = 10;
        position = 10;
        title = "Edit";

        break;
      }

      case StandardMenuID::View: {
        id = 20;
        position = 20;
        title = "View";

        break;
      }

      case StandardMenuID::Help: {
        id = 65000;
        position = 65000;
        title = "Help";

        break;
      }
    }

    // check if it already exists by ID
    for (Size index = 0; index < _menuCount; ++index) {
      if (_menus[index].ID == id) {
        return static_cast<UInt8>(index);
      }
    }

    // create it, temporarily override auto-ID
    UInt8 handle = CreateMenu(title, position);

    if (handle != static_cast<UInt8>(-1)) {
      _menus[handle].ID = id;
    }

    return handle;
  }
}
