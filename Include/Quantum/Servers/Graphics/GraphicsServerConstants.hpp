/**
 * @file Include/Quantum/Servers/Graphics/GraphicsServerConstants.hpp
 * @brief Declares constants for @ref @QGfxSrv.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/ABI.hpp>

namespace Quantum::Servers::Graphics {
  /**
   * @brief ABI version for the graphics server protocol.
   */
  constexpr UInt32 GraphicsABIVersion = 2;

  /**
   * @brief IPC port ID for the graphics server.
   */
  constexpr IPCPortID GraphicsPortID = 4;

  /**
   * @brief Maximum width of the cursor bitmap in pixels.
   */
  constexpr UInt8 MaxCursorWidth = 32;

  /**
   * @brief Maximum height of the cursor bitmap in pixels.
   */
  constexpr UInt8 MaxCursorHeight = 32;
}

namespace Quantum::Servers::Graphics::ABI {
  /**
   * @brief Back-compat alias for @ref Graphics::GraphicsABIVersion.
   *
   * Kept so legacy call sites that reference
   * @c Quantum::Servers::Graphics::ABI::GraphicsABIVersion continue to
   * compile during the refactor. Prefer the unqualified name in the
   * parent namespace.
   */
  using Graphics::GraphicsABIVersion;

  /**
   * @brief Back-compat alias for @ref Graphics::GraphicsPortID.
   */
  using Graphics::GraphicsPortID;
}
