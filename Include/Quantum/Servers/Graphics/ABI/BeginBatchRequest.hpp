/**
 * @file Include/Quantum/Servers/Graphics/ABI/BeginBatchRequest.hpp
 * @brief Declares @ref GraphicsBeginBatchRequest.
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
   * @brief Request to enter batch mode.
   *
   * Subsequent drawing operations update the shadow buffer only,
   * deferring VRAM writes until @ref GraphicsEndBatchRequest.
   */
  struct GraphicsBeginBatchRequest : public GraphicsServerRequest {};
}
