/**
 * @file Servers/Run/Core/ELF/ELFSegmentType.hpp
 * @brief Declares @ref @QRunSrv::Core::ELF::ELFSegmentType.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <RunServerTypes.hpp>

namespace Quantum::Servers::Run::Core::ELF {
  /**
   * @brief ELF program header segment type.
   */
  enum class ELFSegmentType : UInt32 {
    /**
     * @brief Loadable segment.
     */
    Load = 1
  };
}
