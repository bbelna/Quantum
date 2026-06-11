/**
 * @file Servers/Run/Core/Process/ProcessConstants.hpp
 * @brief Declares constants for @ref @QRunSrv::Process.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Servers::Run::Core::Process {
  /**
   * @brief Maximum number of @ref Process entries the run server can track.
   */
  constexpr Size MaxProcessEntries = 64;
}
