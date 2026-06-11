/**
 * @file Include/Quantum/Servers/Graphics/ABI/WriteTextRequest.hpp
 * @brief Declares @ref GraphicsWriteTextRequest.
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
   * @brief Request to write text. The text is stored inline immediately
   *        following the struct header, avoiding cross-process pointer
   *        issues.
   */
  struct GraphicsWriteTextRequest : public GraphicsServerRequest {
    /**
     * @brief Length of the text in bytes (excluding null terminator).
     */
    Size TextLength;

    /**
     * @brief Inline text data (null-terminated, variable length).
     */
    char Text[];
  };
}
