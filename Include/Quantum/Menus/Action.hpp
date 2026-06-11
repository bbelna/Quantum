/**
 * @file Include/Quantum/Menus/Action.hpp
 * @brief Declares action identifiers, action state, and keyboard shortcut
 *        types for the context-oriented menu system.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "MenuTypes.hpp"

namespace Quantum::Menus {
  /**
   * @brief Opaque identifier for an action that can appear in a menu or be
   *        dispatched to an @ref IActionContributor.
   *
   * Well-known system action IDs are defined in @ref WellKnownAction.
   * Application-defined actions use IDs at or above
   * @ref WellKnownAction::AppDefined.
   */
  using ActionID = UInt32;

  /**
   * @brief Well-known system action identifiers.
   *
   * These IDs are reserved by the system for standard operations. Any
   * @ref IActionContributor can handle them. When multiple contributors
   * register the same action, the highest-layer contributor wins.
   */
  enum class WellKnownAction : ActionID {
    /**
     * @brief No action.
     */
    None = 0x0000,

    /**
     * @brief Undo the last operation.
     */
    Undo = 0x0001,

    /**
     * @brief Redo the last undone operation.
     */
    Redo = 0x0002,

    /**
     * @brief Cut the selection to the clipboard.
     */
    Cut = 0x0003,

    /**
     * @brief Copy the selection to the clipboard.
     */
    Copy = 0x0004,

    /**
     * @brief Paste from the clipboard.
     */
    Paste = 0x0005,

    /**
     * @brief Delete the selection.
     */
    Delete = 0x0006,

    /**
     * @brief Select all items in the current context.
     */
    SelectAll = 0x0007,

    /**
     * @brief Show the properties of the selected object.
     */
    Properties = 0x0008,

    /**
     * @brief Open the selected object.
     */
    Open = 0x0009,

    /**
     * @brief Close the current document or view.
     */
    Close = 0x000A,

    /**
     * @brief Save the current document.
     */
    Save = 0x000B,

    /**
     * @brief Save the current document under a new name.
     */
    SaveAs = 0x000C,

    /**
     * @brief Rename the selected object.
     */
    Rename = 0x000D,

    /**
     * @brief Open the find dialog.
     */
    Find = 0x000E,

    /**
     * @brief Open the find-and-replace dialog.
     */
    Replace = 0x000F,

    /**
     * @brief Show contextual help.
     */
    Help = 0x0010,

    /**
     * @brief Show the About dialog.
     */
    About = 0x0011,

    /**
     * @brief First action ID reserved for system menu actions (launch
     *        applications, system settings, etc.).
     *
     * @ref @QCtxSrv::ContextServer handles these directly.
     */
    SystemDefined = 0x0100,

    /**
     * @brief First action ID available for application-defined actions.
     */
    AppDefined = 0x1000
  };

  /**
   * @brief The visual and interaction state of a menu item.
   */
  enum class ActionState : UInt8 {
    /**
     * @brief The action is available and can be invoked.
     */
    Enabled = 0,

    /**
     * @brief The action is visible but cannot be invoked (grayed out).
     */
    Disabled = 1,

    /**
     * @brief The action is enabled and has a check mark.
     */
    Checked = 2,

    /**
     * @brief The action is not shown in the menu.
     */
    Hidden = 3
  };

  /**
   * @brief A keyboard shortcut bound to a menu action.
   */
  struct KeyShortcut {
    /**
     * @brief The scancode or @ref Input::KeyCode constant for the key.
     *        A value of 0 means no shortcut is assigned.
     */
    UInt8 KeyCode = 0;

    /**
     * @brief Modifier flags (bitmask of @ref Input::KeyModifiers).
     */
    UInt8 Modifiers = 0;

    /**
     * @brief Returns whether this shortcut has a key binding.
     */
    bool IsEmpty() const { return KeyCode == 0; }
  };

  /**
   * @brief Capability flags describing what operations an object or
   *        selection supports. Used by contributors to determine which
   *        actions to offer.
   */
  enum class ActionCapability : UInt32 {
    /**
     * @brief No capabilities.
     */
    None = 0,

    /**
     * @brief The selection can be copied.
     */
    CanCopy = 1 << 0,

    /**
     * @brief The selection can be cut.
     */
    CanCut = 1 << 1,

    /**
     * @brief Content can be pasted into the current context.
     */
    CanPaste = 1 << 2,

    /**
     * @brief The selection can be deleted.
     */
    CanDelete = 1 << 3,

    /**
     * @brief The selection can be renamed.
     */
    CanRename = 1 << 4,

    /**
     * @brief The selection can be opened.
     */
    CanOpen = 1 << 5,

    /**
     * @brief The current document can be saved.
     */
    CanSave = 1 << 6,

    /**
     * @brief The last operation can be undone.
     */
    CanUndo = 1 << 7,

    /**
     * @brief The last undone operation can be redone.
     */
    CanRedo = 1 << 8,

    /**
     * @brief Items in the current context can be selected.
     */
    CanSelect = 1 << 9,

    /**
     * @brief The selection supports formatting operations.
     */
    CanFormat = 1 << 10,

    /**
     * @brief The selection can be resized.
     */
    CanResize = 1 << 11,

    /**
     * @brief The selection can be moved.
     */
    CanMove = 1 << 12,

    /**
     * @brief The selection can be grouped with other objects.
     */
    CanGroup = 1 << 13,

    /**
     * @brief The selection can be aligned relative to other objects.
     */
    CanAlign = 1 << 14,

    /**
     * @brief The selection can be exported.
     */
    CanExport = 1 << 15
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(Quantum::Menus, ActionCapability)
