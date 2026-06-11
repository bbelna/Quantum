/**
 * @file Include/Quantum/Clients/ContextClient.hpp
 * @brief Declares @ref @QClients::ContextClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/OS.hpp>
#include <Quantum/Menus.hpp>
#include <Quantum/UI.hpp>

namespace Quantum::Clients {
  /**
   * @brief Client-side interface to the context server.
   *
   * Applications use this class (typically through @ref
   * @QMenus::MenuProvider) to register menu providers, push contributions,
   * and receive action dispatches.
   *
   * @code
   *   ContextClient client;
   *   auto providerID = client.RegisterProvider(MenuLayer::Object, 100);
   *   client.SetContributions(providerID, builder);
   *   client.UpdateFocusContext(focusContext);
   * @endcode
   */
  class ContextClient {
    public:
      /**
       * @brief Creates a new @ref ContextClient instance.
       */
      ContextClient() = default;

      /**
       * @brief Registers a menu provider with the context server.
       * @param layer The menu layer the provider contributes to.
       * @param priority Within-layer priority (higher wins conflicts).
       * @return The assigned provider ID, or 0 on failure.
       */
      Menus::ProviderID RegisterProvider(
        Menus::MenuLayer layer,
        UInt16 priority
      );

      /**
       * @brief Unregisters a menu provider from the context server.
       * @param providerID The provider ID to unregister.
       */
      void UnregisterProvider(Menus::ProviderID providerID);

      /**
       * @brief Pushes a set of menu contributions to the context server.
       * @param providerID The provider ID returned by @ref RegisterProvider.
       * @param builder The populated menu builder containing the
       *        contributions.
       */
      void SetContributions(
        Menus::ProviderID providerID,
        const Menus::MenuBuilder& builder
      );

      /**
       * @brief Notifies the context server that the focus context has
       *        changed.
       * @param context The new focus context.
       */
      void UpdateFocusContext(const Menus::FocusContext& context);

      /**
       * @brief Requests the current composed menu bar state from the
       *        context server.
       * @param outState Pointer to receive the menu bar state.
       * @return `true` if the state was retrieved successfully.
       */
      bool GetMenuBarState(Menus::MenuBarState* outState);

    private:
      /**
       * @brief Computes the byte size needed to serialize a MenuBuilder
       *        into the shared buffer wire format.
       * @param builder The builder to measure.
       * @return Size in bytes.
       */
      UInt32 _computeContributionSize(
        const Menus::MenuBuilder& builder
      );

      /**
       * @brief Serializes a MenuBuilder into a shared buffer.
       * @param builder The builder to serialize.
       * @param destination Pointer to the mapped shared buffer.
       */
      void _serializeContribution(
        const Menus::MenuBuilder& builder,
        UInt8* destination
      );

      /**
       * @brief Deserializes a menu bar state from a shared buffer.
       * @param source Pointer to the mapped shared buffer.
       * @param outState The MenuBarState to populate.
       * @return `true` on success.
       */
      bool _deserializeMenuBarState(
        const UInt8* source,
        Menus::MenuBarState* outState
      );
  };
}
