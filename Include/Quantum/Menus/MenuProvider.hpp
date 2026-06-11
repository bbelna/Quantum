/**
 * @file Include/Quantum/Menus/MenuProvider.hpp
 * @brief Declares @ref @QMenus::MenuProvider.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "FocusContext.hpp"
#include "IActionContributor.hpp"
#include "MenuBuilder.hpp"
#include "MenuTypes.hpp"

namespace Quantum::Menus {
  /**
   * @brief Identifies a registered provider within the context server.
   */
  using ProviderID = UInt32;

  /**
   * @brief The layer at which a provider contributes menus. Higher layers
   *        take priority over lower layers during menu merging.
   */
  enum class MenuLayer : UInt8 {
    /**
     * @brief System-level menus (always present: system menu, Edit, Help).
     */
    System = 0,

    /**
     * @brief Workspace-scoped menus.
     */
    Workspace = 1,

    /**
     * @brief View-level menus contributed by the active window's view.
     */
    View = 2,

    /**
     * @brief Object/selection-level menus contributed by the selected
     *        object or selection.
     */
    Object = 3
  };

  /**
   * @brief Client-side wrapper that manages the lifecycle of a menu
   *        provider registration with the context server.
   *
   * The provider holds a reference to an @ref IActionContributor, which it
   * queries locally when the application's focus context changes. The
   * resulting menu contributions are serialised and pushed to the context
   * server via the @ref ContextClient.
   *
   * Typical usage:
   * @code
   *   MenuProvider provider(MenuLayer::Object, 100);
   *   provider.SetContributor(&myContributor);
   *   provider.Register();
   *
   *   // when selection changes:
   *   provider.Refresh(currentContext);
   *
   *   // on shutdown:
   *   provider.Unregister();
   * @endcode
   */
  class MenuProvider {
    public:
      /**
       * @brief Constructs a menu provider for the given layer and priority.
       * @param layer The menu layer this provider contributes to.
       * @param priority Within-layer ordering (higher = wins conflicts).
       */
      explicit MenuProvider(MenuLayer layer, UInt16 priority);

      /**
       * @brief Destroys the provider. Automatically unregisters if still
       *        registered.
       */
      ~MenuProvider();

      /**
       * @brief Sets the contributor that provides menu content.
       * @param contributor Pointer to the contributor. Must remain valid
       *        for the lifetime of this provider, or until replaced.
       */
      void SetContributor(IActionContributor* contributor);

      /**
       * @brief Registers this provider with the context server.
       * @return `true` if registration succeeded.
       */
      bool Register();

      /**
       * @brief Unregisters this provider from the context server.
       */
      void Unregister();

      /**
       * @brief Returns whether this provider is currently registered.
       */
      bool IsRegistered() const { return _providerID != 0; }

      /**
       * @brief Queries the contributor for fresh menu contributions and
       *        pushes them to the context server.
       *
       * Call this whenever the application's internal state changes in a
       * way that would affect the menus (e.g. selection changed, document
       * modified, undo stack updated).
       *
       * @param context The current focus context.
       */
      void Refresh(const FocusContext& context);

      /**
       * @brief Dispatches an action to the contributor.
       * @param action The action ID to execute.
       * @param context The focus context at invocation time.
       * @return `true` if the contributor handled the action.
       */
      bool DispatchAction(ActionID action, const FocusContext& context);

      /**
       * @brief Returns the provider ID assigned by the context server,
       *        or 0 if not registered.
       */
      ProviderID GetProviderID() const { return _providerID; }

    private:
      /**
       * @brief The contributor that provides menu content.
       */
      IActionContributor* _contributor = nullptr;

      /**
       * @brief The menu layer this provider contributes to.
       */
      MenuLayer _layer;

      /**
       * @brief Within-layer priority (higher wins conflicts).
       */
      UInt16 _priority;

      /**
       * @brief Provider ID assigned by the context server on registration.
       *        0 means not registered.
       */
      ProviderID _providerID = 0;

      /**
       * @brief Reusable builder instance to avoid per-refresh allocation.
       */
      MenuBuilder _builder;
  };
}
