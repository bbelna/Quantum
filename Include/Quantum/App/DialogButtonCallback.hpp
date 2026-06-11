/**
 * @file Include/Quantum/App/DialogButtonCallback.hpp
 * @brief Declares @ref @QApp::DialogButtonCallback.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::App {
  class Dialog;

  /**
   * @brief Callback type for dialog button clicks.
   * @param dialog Reference to the dialog that owns the clicked button.
   *               The handler may call @ref Dialog @ref Dialog::Close to
   *               dismiss it.
   */
  using DialogButtonCallback = void (*)(Dialog& dialog);
}
