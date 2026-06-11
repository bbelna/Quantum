/**
 * @file Menus/MenuProvider.cpp
 * @brief Implements @ref @QMenus::MenuProvider.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Clients/ContextClient.hpp>
#include <Quantum/Menus/MenuProvider.hpp>

#include "MenuTypes.hpp"

namespace Quantum::Menus {
  MenuProvider::MenuProvider(
    MenuLayer layer,
    UInt16 priority
  ) :
    _layer(layer),
    _priority(priority)
  {
  }

  MenuProvider::~MenuProvider() {
    if (IsRegistered()) {
      Unregister();
    }
  }

  void MenuProvider::SetContributor(IActionContributor* contributor) {
    _contributor = contributor;
  }

  bool MenuProvider::Register() {
    if (IsRegistered()) {
      return true;
    } else {
      ProviderID assignedID = Context.RegisterProvider(
        _layer,
        _priority
      );

      if (assignedID > 0) {
        _providerID = assignedID;

        return true;
      } else {
        return false;
      }
    }
  }

  void MenuProvider::Unregister() {
    if (IsRegistered()) {
      Context.UnregisterProvider(_providerID);

      _providerID = 0;
    }
  }

  void MenuProvider::Refresh(const FocusContext& context) {
    if (IsRegistered() && _contributor) {
      _builder.Reset();

      _contributor->ContributeMenuItems(
        context,
        _builder
      );

      Context.SetContributions(
        _providerID,
        _builder
      );
    }
  }

  bool MenuProvider::DispatchAction(
    ActionID action,
    const FocusContext& context
  ) {
    return _contributor
      ? _contributor->ExecuteAction(action, context)
      : false;
  }
}
