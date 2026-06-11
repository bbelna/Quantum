/**
 * @file Include/Quantum/Menus/MenuItem.hpp
 * @brief Declares the @ref @QMenus::MenuItem struct and related types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "MenuTypes.hpp"
#include "Action.hpp"

namespace Quantum::Menus {
  /**
   * @brief Maximum length of a menu item label, including the null terminator.
   */
  static constexpr Size MaxMenuItemLabelLength = 48;

  /**
   * @brief Identifies one of the always-present standard menus.
   *
   * Standard menus occupy fixed positions in the menu bar. Object-specific
   * menus created by contributors are inserted between @ref View and
   * @ref Help.
   */
  enum class StandardMenuID : UInt8 {
    /**
     * @brief The system menu (leftmost, identified by the system icon).
     */
    System = 0,

    /**
     * @brief The Edit menu (Undo, Cut, Copy, Paste, etc.).
     */
    Edit = 1,

    /**
     * @brief The View menu (zoom, layout, display options).
     */
    View = 2,

    /**
     * @brief The Help menu (always rightmost).
     */
    Help = 255
  };

  /**
   * @brief A single item within a menu.
   *
   * This is a plain data structure suitable for IPC transfer. Menu items
   * are collected by the @ref MenuBuilder and serialised into
   * @ref MenuContribution structures for transport to the context server.
   */
  struct MenuItem {
    /**
     * @brief The action identifier. When the user selects this item, this
     *        ID is dispatched back to the contributing provider.
     */
    ActionID Action = static_cast<ActionID>(WellKnownAction::None);

    /**
     * @brief The display label for the menu item.
     */
    char Label[MaxMenuItemLabelLength] = {};

    /**
     * @brief Optional keyboard shortcut displayed alongside the label.
     */
    KeyShortcut Shortcut = {};

    /**
     * @brief The initial state of this menu item.
     */
    ActionState State = ActionState::Enabled;

    /**
     * @brief Relative ordering hint within the menu. Lower values appear
     *        higher. Items with equal hints preserve insertion order.
     */
    UInt16 OrderHint = 0;

    /**
     * @brief If true, a separator line is drawn above this item.
     */
    bool SeparatorBefore = false;
  };
}
