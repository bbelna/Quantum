/**
 * @file Menus/MenuBar.cpp
 * @brief Implements @ref @QMenus::Menu and @ref @QMenus::MenuBarState.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Menus/MenuBar.hpp>

#include "MenuTypes.hpp"

namespace Quantum::Menus {
  bool Menu::AddItem(const MenuItem& item) {
    if (ItemCount >= ItemCapacity) {
      Size newCapacity = (ItemCapacity == 0) ? 4 : ItemCapacity * 2;
      MenuItem* newItems = new MenuItem[newCapacity];

      for (
        Size index = 0;
        index < ItemCount;
        ++index
      ) {
        newItems[index] = Items[index];
      }

      delete[] Items;

      Items = newItems;
      ItemCapacity = newCapacity;
    }

    Items[ItemCount] = item;
    ItemCount++;

    return true;
  }

  void Menu::FreeItems() {
    delete[] Items;

    Items = nullptr;
    ItemCount = 0;
    ItemCapacity = 0;
  }

  Menu* MenuBarState::FindOrCreateMenu(
    MenuID id,
    UInt16 position,
    const char* title
  ) {
    // look for existing menu with matching ID
    for (
      Size index = 0;
      index < MenuCount;
      ++index
    ) {
      if (Menus[index].ID == id) {
        return &Menus[index];
      }
    }

    // grow if needed
    if (MenuCount >= MenuCapacity) {
      Size newCapacity
        = MenuCapacity == 0
        ? 4
        : MenuCapacity * 2;
      Menu* newMenus = new Menu[newCapacity];

      for (
        Size index = 0;
        index < MenuCount;
        ++index
      ) {
        newMenus[index] = Menus[index];

        // clear source so it doesn't double-free items on destruction
        Menus[index].Items = nullptr;
        Menus[index].ItemCount = 0;
        Menus[index].ItemCapacity = 0;
      }

      delete[] Menus;

      Menus = newMenus;
      MenuCapacity = newCapacity;
    }

    Menu& menu = Menus[MenuCount];

    menu = Menu{};
    menu.ID = id;
    menu.Position = position;

    CString::Copy(
      title,
      menu.Title,
      MaxMenuTitleLength
    );

    MenuCount++;

    return &menu;
  }

  void MenuBarState::SortByPosition() {
    // simple insertion sort, menu count is small
    for (
      Size outerIndex = 1;
      outerIndex < MenuCount;
      ++outerIndex
    ) {
      // swap by moving pointers, not copying entire structs
      Menu temporary = Menus[outerIndex];

      // detach source so it won't free items
      Menus[outerIndex].Items = nullptr;
      Menus[outerIndex].ItemCount = 0;
      Menus[outerIndex].ItemCapacity = 0;

      Size insertIndex = outerIndex;

      // focus menus sort before non-focus; within each group, sort
      // by position
      while (insertIndex > 0) {
        bool previousIsFocus = Menus[insertIndex - 1].IsFocusMenu;
        bool currentIsFocus = temporary.IsFocusMenu;

        // focus menus always come first
        if (
          currentIsFocus &&
          !previousIsFocus
        ) {
          // current is focus, previous is not, keep moving left
        } else if (
          !currentIsFocus &&
          previousIsFocus
        ) {
          break; // previous is focus, current is not, stop
        } else if (
          Menus[insertIndex - 1].Position <= temporary.Position
        ) {
          break; // same group, previous has lower/equal position, stop
        }

        // else: same group, previous has higher position, swap
        Menus[insertIndex] = Menus[insertIndex - 1];
        Menus[insertIndex - 1].Items = nullptr;
        Menus[insertIndex - 1].ItemCount = 0;
        Menus[insertIndex - 1].ItemCapacity = 0;

        insertIndex--;
      }

      Menus[insertIndex] = temporary;
    }
  }

  void MenuBarState::FreeAll() {
    for (
      Size index = 0;
      index < MenuCount;
      ++index
    ) {
      Menus[index].FreeItems();
    }

    delete[] Menus;

    Menus = nullptr;
    MenuCount = 0;
    MenuCapacity = 0;
  }
}
