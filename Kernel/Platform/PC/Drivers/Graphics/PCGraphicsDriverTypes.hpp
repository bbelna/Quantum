/**
 * @file
 *   Kernel/Platform/PC/Drivers/Graphics/PCGraphicsDriverTypes.hpp
 * @brief Declares @ref @QKrnlPC::Drivers::Graphics types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Platform::PC::Drivers::Graphics {
  class S3ViRGEDriver;
  class VESADriver;
  class VGADriver;

  /**
   * @brief Color used by PC graphics drivers to blank the framebuffer
   *        and as the kernel text-mode console's background.
   */
  static constexpr UInt32 ClearColor = 0xFF000000;
}
