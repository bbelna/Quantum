/**
 * @file Include/Quantum/Menus/IActionContributor.hpp
 * @brief Declares the @ref @QMenus::IActionContributor interface.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Action.hpp"
#include "FocusContext.hpp"

namespace Quantum::Menus {
  class MenuBuilder;

  /**
   * @brief Interface for objects that contribute menu items based on the
   *        current focus context.
   *
   * Applications implement this interface to participate in the
   * context-oriented menu system. A contributor is associated with a
   * @ref MenuProvider, which handles registration with the context server
   * and serialises the contributor's output for IPC transport.
   *
   * Contributors are queried locally by the @ref MenuProvider whenever the
   * application's focus context changes. The provider then pushes the
   * serialised result to the context server.
   */
  class IActionContributor {
    public:
      virtual ~IActionContributor() = default;

      /**
       * @brief Populates menus for the given focus context.
       *
       * Called by the owning @ref MenuProvider when the application needs
       * to rebuild its menu contributions (e.g. after a selection change).
       *
       * @param context The current focus context.
       * @param builder The builder to add menu items to.
       */
      virtual void ContributeMenuItems(
        const FocusContext& context,
        MenuBuilder& builder
      ) = 0;

      /**
       * @brief Executes an action by ID.
       *
       * Called when the user selects a menu item or presses a keyboard
       * shortcut that maps to an action contributed by this object.
       *
       * @param action The action to execute.
       * @param context The focus context at the time of invocation.
       * @return `true` if the action was handled.
       */
      virtual bool ExecuteAction(
        ActionID action,
        const FocusContext& context
      ) = 0;

      /**
       * @brief Queries the current state of an action.
       *
       * Called to determine whether an action should appear enabled,
       * disabled, checked, or hidden in the menu.
       *
       * @param action The action to query.
       * @param context The current focus context.
       * @return The action's current state.
       */
      virtual ActionState QueryActionState(
        ActionID action,
        const FocusContext& context
      ) = 0;
  };
}
