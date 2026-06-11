/**
 * @file Servers/Context/MenuService.hpp
 * @brief Declares @ref @QCtxSrv::MenuService.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <ContextServerTypes.hpp>

namespace Quantum::Servers::Context {
  /**
   * @brief Maximum number of simultaneously registered menu providers.
   */
  static constexpr Size MaxProviders = 32;

  /**
   * @brief Internal record of a registered menu provider.
   */
  struct RegisteredProvider {
    /**
     * @brief The provider ID assigned at registration. 0 means this slot
     *        is unused.
     */
    ProviderID ProviderID = 0;

    /**
     * @brief The process ID of the owning application.
     */
    UInt32 ProcessID = 0;

    /**
     * @brief The menu layer this provider contributes to.
     */
    MenuLayer Layer = MenuLayer::System;

    /**
     * @brief Within-layer priority (higher wins conflicts).
     */
    UInt16 Priority = 0;

    /**
     * @brief The most recently pushed contribution, dynamically allocated.
     *        Owned by this provider slot; freed on unregister or update.
     */
    MenuBarState* Contribution = nullptr;

    /**
     * @brief Suppressed action IDs from this provider.
     */
    ActionID* SuppressedActions = nullptr;

    /**
     * @brief Number of suppressed action IDs.
     */
    Size SuppressedCount = 0;

    /**
     * @brief Whether this provider has pushed at least one contribution.
     */
    bool HasContribution = false;

    /**
     * @brief Frees the contribution and suppressed arrays.
     */
    void FreeContribution();
  };

  /**
   * @brief Maximum number of action-to-provider mappings tracked in the
   *        composed menu bar.
   */
  static constexpr Size MaxTrackedActions = 256;

  /**
   * @brief Maps an action ID in the composed menu bar back to the provider
   *        that contributed it.
   */
  struct ActionProviderMapping {
    /**
     * @brief The action ID.
     */
    ActionID Action = 0;

    /**
     * @brief The provider ID that contributed this action.
     */
    ProviderID Provider = 0;

    /**
     * @brief The process ID of the contributing provider.
     */
    UInt32 ProcessID = 0;
  };

  /**
   * @brief Manages menu provider registrations, contribution merging, and
   *        action dispatch within the context server.
   *
   * The menu service maintains a table of registered providers and the
   * current focus context. When the context changes or a provider pushes
   * new contributions, the service rebuilds the composed
   * @ref MenuBarState by merging contributions from all active providers
   * according to layer precedence and priority rules.
   */
  class MenuService {
    public:
      /**
       * @brief Constructs the menu service.
       * @param kernel Reference to the @ref KernelClient for shared buffer
       *               operations. Must outlive this service.
       * @param log Reference to the @ref ServerLog for logging. Must outlive
       *            this service.
       */
      MenuService(KernelClient& kernel, ServerLog& log)
        : _kernel(kernel), _log(log) {}

      /**
       * @brief Destroys the menu service and frees all contributions.
       */
      ~MenuService();

      /**
       * @brief Registers a new menu provider.
       * @param processID The owning process.
       * @param layer The menu layer.
       * @param priority Within-layer priority.
       * @return The assigned provider ID, or 0 if registration failed.
       */
      ProviderID RegisterProvider(
        UInt32 processID,
        MenuLayer layer,
        UInt16 priority
      );

      /**
       * @brief Unregisters a menu provider.
       * @param providerID The provider ID to unregister.
       */
      void UnregisterProvider(ProviderID providerID);

      /**
       * @brief Stores a contribution deserialized from a shared buffer
       *        and triggers a menu bar rebuild.
       * @param providerID The provider that sent the contributions.
       * @param bufferID The shared buffer containing the wire-format data.
       */
      void SetContributionsFromBuffer(
        ProviderID providerID,
        SharedBufferID bufferID
      );

      /**
       * @brief Updates the current focus context and triggers a menu bar
       *        rebuild.
       * @param context The new focus context.
       */
      /**
       * @brief Registers a built-in system menu that always appears in the
       *        menu bar regardless of focus state.
       * @param menu The menu to add. Ownership of items is transferred.
       */
      void AddSystemMenu(Menu&& menu);

      void UpdateFocusContext(const FocusContext& context);

      /**
       * @brief Returns the current focus context.
       */
      const FocusContext& GetFocusContext() const { return _currentContext; }

      /**
       * @brief Returns a reference to the current composed menu bar state.
       */
      const MenuBarState& GetMenuBarState() const { return *_currentBar; }

      /**
       * @brief Serializes the current menu bar state into a new shared
       *        buffer.
       * @return The SharedBufferID, or 0 on failure. The caller (or
       *         recipient) is responsible for detaching.
       */
      SharedBufferID SerializeMenuBarState() const;

      /**
       * @brief Finds the provider that contributed a given action ID.
       * @param action The action ID to look up.
       * @return Pointer to the mapping, or `nullptr` if the action is not
       *         in the current menu bar.
       */
      const ActionProviderMapping* FindProviderForAction(
        ActionID action
      ) const;

    private:
      /**
       * @brief Reference to the kernel client for shared buffer operations.
       */
      KernelClient& _kernel;

      /**
       * @brief Reference to the server log.
       */
      ServerLog& _log;

      /**
       * @brief Registered provider table.
       */
      RegisteredProvider _providers[MaxProviders] = {};

      /**
       * @brief Next provider ID to assign.
       */
      ProviderID _nextProviderID = 1;

      /**
       * @brief The current focus context.
       */
      FocusContext _currentContext = {};

      /**
       * @brief System menus that always appear regardless of focus.
       */
      Menu* _systemMenus = nullptr;

      /**
       * @brief Number of system menus.
       */
      Size _systemMenuCount = 0;

      /**
       * @brief Capacity of the system menu array.
       */
      Size _systemMenuCapacity = 0;

      /**
       * @brief The current composed menu bar (heap-allocated).
       */
      MenuBarState* _currentBar = nullptr;

      /**
       * @brief Maps each action in the composed menu bar to the provider
       *        that contributed it.
       */
      ActionProviderMapping _actionMap[MaxTrackedActions] = {};

      /**
       * @brief Number of valid entries in @ref _actionMap.
       */
      Size _actionMapCount = 0;

      /**
       * @brief Address of the last serialized menu bar shared buffer, or 0.
       *        Kept alive until the next serialization so the client has time
       *        to attach.
       */
      UIntPtr _lastSerializedBuffer = 0;

      /**
       * @brief Detaches the previous serialized buffer (if any).
       */
      void _detachPreviousSerializedBuffer();

      /**
       * @brief Rebuilds the composed menu bar from all active providers.
       */
      void _rebuildMenuBar();

      /**
       * @brief Merges a single provider's contribution into a menu bar
       *        being built.
       * @param bar The menu bar to merge into.
       * @param contribution The contribution to merge.
       * @param providerID The contributing provider's ID.
       * @param processID The contributing provider's process ID.
       */
      void _mergeContribution(
        MenuBarState* bar,
        const RegisteredProvider& provider
      );

      /**
       * @brief Computes a simple hash of the menu bar state for change
       *        detection.
       */
      UInt32 _computeContentHash() const;
  };
}
