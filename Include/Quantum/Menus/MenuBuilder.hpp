/**
 * @file Include/Quantum/Menus/MenuBuilder.hpp
 * @brief Declares the @ref @QMenus::MenuBuilder class.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "MenuTypes.hpp"
#include "MenuBar.hpp"
#include "MenuItem.hpp"

namespace Quantum::Menus {
  /**
   * @brief Builds a set of menu contributions from an @ref IActionContributor.
   *
   * The builder accumulates menus and items into dynamically allocated
   * internal storage. After the contributor has finished populating it, the
   * owning @ref MenuProvider serialises the result for transport to the
   * context server via a shared buffer.
   *
   * Contributors interact with the builder through two patterns:
   *   - Adding items to standard menus (@ref AddToStandardMenu).
   *   - Creating context-specific menus (@ref CreateMenu, @ref AddItem).
   */
  class MenuBuilder {
    public:
      /**
       * @brief Constructs an empty builder, ready for population.
       */
      MenuBuilder() = default;

      /**
       * @brief Destroys the builder and frees all allocated menus and items.
       */
      ~MenuBuilder();

      /**
       * @brief Resets the builder to its initial empty state, freeing all
       *        allocated menus and items.
       */
      void Reset();

      /**
       * @brief Adds an item to one of the always-present standard menus.
       * @param menu The standard menu to contribute to.
       * @param item The menu item to add.
       */
      void AddToStandardMenu(StandardMenuID menu, const MenuItem& item);

      /**
       * @brief Creates a new context-specific menu.
       *
       * The menu appears between View and Help in the menu bar. Multiple
       * menus from different contributors with the same title are merged.
       *
       * @param title The menu title displayed in the menu bar.
       * @param position Ordering hint (lower = further left).
       * @return A handle used with @ref AddItem, or
       *         `static_cast<UInt8>(-1)` if allocation failed.
       */
      UInt8 CreateMenu(const char* title, UInt16 position);

      /**
       * @brief Creates a focus menu, rendered bold at the leftmost
       *        position. Represents the focused window or object.
       *
       * Focus menus always sort before non-focus menus regardless of
       * position value.
       *
       * @param title The menu title (typically the app or object name).
       * @return A handle used with @ref AddItem, or
       *         `static_cast<UInt8>(-1)` if allocation failed.
       */
      UInt8 CreateFocusMenu(const char* title);

      /**
       * @brief Adds an item to a previously created menu.
       * @param menuHandle The handle returned by @ref CreateMenu.
       * @param item The menu item to add.
       */
      void AddItem(UInt8 menuHandle, const MenuItem& item);

      /**
       * @brief Marks an action for suppression in lower layers.
       *
       * When the context server merges contributions, suppressed actions
       * contributed by lower-priority providers are removed from the final
       * menu bar.
       *
       * @param action The action ID to suppress.
       */
      void SuppressAction(ActionID action);

      /**
       * @brief Returns the number of menus built so far.
       */
      Size GetMenuCount() const { return _menuCount; }

      /**
       * @brief Returns a pointer to the internal menu array.
       */
      const Menu* GetMenus() const { return _menus; }

      /**
       * @brief Returns the number of suppressed actions.
       */
      Size GetSuppressedCount() const { return _suppressedCount; }

      /**
       * @brief Returns a pointer to the suppressed action ID array.
       */
      const ActionID* GetSuppressedActions() const { return _suppressed; }

    private:
      /**
       * @brief Pointer to the dynamically allocated menu array.
       */
      Menu* _menus = nullptr;

      /**
       * @brief Number of valid menus in @ref _menus.
       */
      Size _menuCount = 0;

      /**
       * @brief Allocated capacity of @ref _menus.
       */
      Size _menuCapacity = 0;

      /**
       * @brief Pointer to the dynamically allocated suppressed action array.
       */
      ActionID* _suppressed = nullptr;

      /**
       * @brief Number of valid entries in @ref _suppressed.
       */
      Size _suppressedCount = 0;

      /**
       * @brief Next menu ID to assign for custom menus.
       */
      MenuID _nextMenuID = 100;

      /**
       * @brief Allocated capacity of @ref _suppressed.
       */
      Size _suppressedCapacity = 0;

      /**
       * @brief Grows the menu array if needed.
       * @return `true` if there is room for a new menu.
       */
      bool _growMenus();

      /**
       * @brief Grows the suppressed action array if needed.
       * @return `true` if there is room for a new entry.
       */
      bool _growSuppressed();

      /**
       * @brief Finds or creates the internal menu for a standard menu ID.
       * @param menu The standard menu ID.
       * @return Index into @ref _menus, or `static_cast<UInt8>(-1)` on
       *         failure.
       */
      UInt8 _findOrCreateStandardMenu(StandardMenuID menu);
  };
}
