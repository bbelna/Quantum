/**
 * @file Include/Quantum/Menus/FocusContext.hpp
 * @brief Declares focus context types that describe the user's current
 *        interaction target for the context-oriented menu system.
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
   * @brief Identifies a type of object that can be selected in a view.
   *
   * Each application registers its own type IDs starting at
   * @ref WellKnownObjectType::AppDefined.
   */
  using ObjectTypeID = UInt32;

  /**
   * @brief Well-known object type identifiers.
   */
  enum class WellKnownObjectType : ObjectTypeID {
    /**
     * @brief No object type.
     */
    None = 0x0000,

    /**
     * @brief A file.
     */
    File = 0x0001,

    /**
     * @brief A directory.
     */
    Directory = 0x0002,

    /**
     * @brief A text selection or text run.
     */
    Text = 0x0003,

    /**
     * @brief An image or bitmap object.
     */
    Image = 0x0004,

    /**
     * @brief A storage volume.
     */
    Volume = 0x0005,

    /**
     * @brief First object type ID available for application-defined types.
     */
    AppDefined = 0x1000
  };

  /**
   * @brief Describes the kind of selection the user has made.
   */
  enum class SelectionKind : UInt8 {
    /**
     * @brief Nothing is selected.
     */
    None = 0,

    /**
     * @brief A single discrete object is selected.
     */
    Single = 1,

    /**
     * @brief Multiple discrete objects are selected.
     */
    Multi = 2,

    /**
     * @brief A contiguous range is selected (e.g. text span).
     */
    Range = 3
  };

  /**
   * @brief Maximum number of distinct object types tracked in a
   *        heterogeneous selection.
   */
  static constexpr Size MaxSelectionTypes = 8;

  /**
   * @brief Describes the objects currently selected or focused by the user.
   */
  struct ObjectContext {
    /**
     * @brief The kind of selection.
     */
    SelectionKind Kind = SelectionKind::None;

    /**
     * @brief The type of the primary (most specific) selected object.
     */
    ObjectTypeID PrimaryType = static_cast<ObjectTypeID>(
      WellKnownObjectType::None
    );

    /**
     * @brief All distinct types present in the selection. For a homogeneous
     *        selection only index 0 is meaningful.
     */
    ObjectTypeID Types[MaxSelectionTypes] = {};

    /**
     * @brief Number of valid entries in @ref Types.
     */
    UInt8 TypeCount = 0;

    /**
     * @brief Total number of selected objects.
     */
    UInt32 ObjectCount = 0;

    /**
     * @brief Bitmask of capabilities common to all selected objects.
     */
    ActionCapability Capabilities = ActionCapability::None;

    /**
     * @brief Returns whether the selection contains objects of the given type.
     * @param type The object type to check for.
     */
    bool HasType(ObjectTypeID type) const {
      for (UInt8 index = 0; index < TypeCount; ++index) {
        if (Types[index] == type) return true;
      }

      return false;
    }

    /**
     * @brief Returns whether the selection has a specific capability.
     * @param capability The capability to test.
     */
    bool HasCapability(ActionCapability capability) const {
      return (Capabilities & capability) == capability;
    }
  };

  /**
   * @brief Identifies a workspace managed by the window manager.
   */
  using WorkspaceID = UInt32;

  /**
   * @brief Identifies a view within a window (tab, pane, split).
   */
  using ViewID = UInt32;

  /**
   * @brief Complete description of the user's current interaction context.
   *
   * Updated by the application server (window focus) and by applications
   * (selection changes). The context server uses this to determine which
   * menu providers are active and to merge their contributions into the
   * system menu bar.
   */
  struct FocusContext {
    /**
     * @brief The active workspace.
     */
    WorkspaceID Workspace = 0;

    /**
     * @brief The focused window's resource ID (from the application server).
     */
    UInt32 WindowID = 0;

    /**
     * @brief The active view within the focused window.
     */
    ViewID View = 0;

    /**
     * @brief The process ID that owns the focused window.
     */
    UInt32 ProcessID = 0;

    /**
     * @brief The object/selection context within the focused view.
     */
    ObjectContext Object;
  };
}
