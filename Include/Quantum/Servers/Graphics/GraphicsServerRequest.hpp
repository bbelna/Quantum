/**
 * @file Include/Quantum/Servers/Graphics/GraphicsServerRequest.hpp
 * @brief Base request type aliases for the graphics server.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/ABI.hpp>

#include "GraphicsServerOperation.hpp"

namespace Quantum::Servers::Graphics {
  /**
   * @brief Base request type for fire-and-forget graphics operations.
   */
  using GraphicsServerRequest = ABIRequest<GraphicsServerOperation>;

  /**
   * @brief Base request type for graphics operations that expect a reply.
   */
  using GraphicsServerRequestWithReply
    = ABIRequestWithReplyPort<GraphicsServerOperation>;
}

namespace Quantum::Servers::Graphics::ABI {
  /**
   * @brief Back-compat alias for @ref Graphics::GraphicsServerRequest.
   *
   * Kept so legacy call sites that reference
   * @c Quantum::Servers::Graphics::ABI::GraphicsRequest continue to
   * compile during the refactor. Prefer @ref GraphicsServerRequest.
   */
  using GraphicsRequest = Graphics::GraphicsServerRequest;

  /**
   * @brief Back-compat alias for @ref Graphics::GraphicsServerRequestWithReply.
   */
  using GraphicsRequestWithReply = Graphics::GraphicsServerRequestWithReply;
}
