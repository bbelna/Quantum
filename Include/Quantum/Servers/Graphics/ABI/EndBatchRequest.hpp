/**
 * @file Include/Quantum/Servers/Graphics/ABI/EndBatchRequest.hpp
 * @brief Declares @ref GraphicsEndBatchRequest.
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
   * @brief Request to exit batch mode.
   *
   * Flushes the accumulated dirty region from the shadow buffer to VRAM
   * in a single operation.
   */
  struct GraphicsEndBatchRequest : public GraphicsServerRequest {};
}
