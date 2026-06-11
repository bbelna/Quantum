/**
 * @file Include/Quantum/Servers/Graphics/ABI/GetModeInfoRequest.hpp
 * @brief Declares @ref GraphicsGetModeInfoRequest.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "../GraphicsServerRequest.hpp"

namespace Quantum::Servers::Graphics::ABI {
  /**
   * @brief Request to query the current video mode information.
   */
  struct GraphicsGetModeInfoRequest : public GraphicsServerRequestWithReply {};
}
