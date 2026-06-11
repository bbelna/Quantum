/**
 * @file Include/Quantum/Cursors/CursorType.hpp
 * @brief Declares @ref @QCursors::CursorType.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Cursors {
  /**
   * @brief Identifies a system cursor within a QCUR cursor set.
   *
   * The numeric value of each enumerator corresponds to the cursor's
   * index within the QCUR file.
   */
  enum class CursorType : UInt8 {
    /**
     * @brief Standard arrow pointer.
     */
    Arrow = 0,

    /**
     * @brief Text insertion I-beam.
     */
    IBeam = 1,

    /**
     * @brief Precision crosshair.
     */
    Crosshair = 2,

    /**
     * @brief Busy/wait indicator.
     */
    Watch = 3,

    /**
     * @brief Open hand (link/drag).
     */
    Hand = 4,

    /**
     * @brief NW-SE diagonal resize.
     */
    ResizeNWSE = 5,

    /**
     * @brief NE-SW diagonal resize.
     */
    ResizeNESW = 6,

    /**
     * @brief Vertical resize.
     */
    ResizeNS = 7,

    /**
     * @brief Horizontal resize.
     */
    ResizeEW = 8,
  };

  /**
   * @brief Total number of cursor types in the set.
   */
  static constexpr UInt8 CursorTypeCount = 9;
}
