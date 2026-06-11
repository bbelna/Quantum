/**
 * @file Clients/StartupClient.cpp
 * @brief Implements @ref @QClients::StartupClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ClientsTypes.hpp>
#include "StartupClient.hpp"

namespace Quantum::Clients {
  void StartupClient::Ready() {
    StartupRequest request {
      .ABIVersion = StartupABIVersion,
      .Operation = StartupOperation::Ready
    };

    SendOS(StartupPortID, request);
  }
}
