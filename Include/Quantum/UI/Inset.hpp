/**
 * @file Include/Quantum/UI/Inset.hpp
 * @brief Declares @ref @QUI::Inset.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "FrameT.hpp"

namespace Quantum::UI {
  /**
   * @brief Represents an inset with pixel dimensions for each edge.
   * @see @ref FrameT
   *
   * Wraps @ref FrameT<Size>. Used for padding and margin in @ref Container
   * and @ref Border.
   */
  struct Inset : public FrameT<Size> {};
}
