/**
 * @file Include/Quantum/Menus/MenuBar.hpp
 * @brief Declares the @ref @QMenus::Menu and @ref @QMenus::MenuBarState
 *        types representing the composed system menu bar.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "MenuTypes.hpp"
#include "MenuItem.hpp"

namespace Quantum::Menus {
  /**
   * @brief Maximum length of a menu title, including the null terminator.
   */
  static constexpr Size MaxMenuTitleLength = 32;

  /**
   * @brief A single drop-down menu in the menu bar.
   *
   * Items are dynamically allocated. The menu owns the item array and
   * frees it on destruction.
   */
  /**
   * @brief Unique identifier for a menu within the menu bar.
   */
  using MenuID = UInt16;

  struct Menu {
    /**
     * @brief Unique identifier for this menu. Used by FindOrCreateMenu
     *        to match existing menus during merging.
     */
    MenuID ID = 0;

    /**
     * @brief The title displayed in the menu bar for this menu.
     */
    char Title[MaxMenuTitleLength] = {};

    /**
     * @brief Ordering position in the menu bar. Lower values appear further
     *        left. Standard menus use well-known positions; object menus
     *        are inserted between View and Help.
     */
    UInt16 Position = 0;

    /**
     * @brief If true, this is a focus menu, rendered bold at the
     *        leftmost position. Focus menus represent the currently
     *        focused window or object and appear/disappear based on
     *        focus state.
     */
    bool IsFocusMenu = false;

    /**
     * @brief Pointer to the dynamically allocated item array, or `nullptr`.
     */
    MenuItem* Items = nullptr;

    /**
     * @brief Number of valid items in @ref Items.
     */
    Size ItemCount = 0;

    /**
     * @brief Allocated capacity of @ref Items.
     */
    Size ItemCapacity = 0;

    /**
     * @brief Appends an item, growing the array if needed.
     * @param item The item to append.
     * @return `true` if the item was added.
     */
    bool AddItem(const MenuItem& item);

    /**
     * @brief Releases the item array.
     */
    void FreeItems();
  };

  /**
   * @brief The complete state of the system menu bar, as composed by the
   *        context server from all active providers.
   *
   * Menus are dynamically allocated. The bar owns the menu array and
   * frees it (along with each menu's items) on destruction.
   */
  struct MenuBarState {
    /**
     * @brief Pointer to the dynamically allocated menu array, or `nullptr`.
     */
    Menu* Menus = nullptr;

    /**
     * @brief Number of valid menus in @ref Menus.
     */
    Size MenuCount = 0;

    /**
     * @brief Allocated capacity of @ref Menus.
     */
    Size MenuCapacity = 0;

    /**
     * @brief A content hash of the menu bar state, used to detect no-change
     *        scenarios and avoid unnecessary redraws.
     */
    UInt32 ContentHash = 0;

    /**
     * @brief Finds or creates a menu by ID.
     * @param id The unique menu identifier.
     * @param position The ordering position (used only when creating).
     * @param title The title (used only when creating).
     * @return Pointer to the menu, or `nullptr` if allocation failed.
     */
    Menu* FindOrCreateMenu(MenuID id, UInt16 position, const char* title);

    /**
     * @brief Sorts the menus by position (ascending).
     */
    void SortByPosition();

    /**
     * @brief Releases all menus and their items.
     */
    void FreeAll();
  };
}
