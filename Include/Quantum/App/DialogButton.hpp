/**
 * @file Include/Quantum/App/DialogButton.hpp
 * @brief Declares @ref @QApp::DialogButton.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "DialogButtonCallback.hpp"

namespace Quantum::App {
  /**
   * @brief Describes a button in a @ref Dialog.
   *
   * Passed to the @ref Dialog constructor to configure button layout and
   * behavior.
   */
  struct DialogButton {
    /**
     * @brief The text label displayed on the button.
     */
    char Label[32] = {};

    /**
     * @brief Callback invoked when the @ref DialogButton is clicked.
     *
     * Receives a reference to the owning @ref Dialog so the handler can
     * close it. May be `nullptr` for a no-op button.
     */
    DialogButtonCallback OnClick = nullptr;

    /**
     * @brief Creates a @ref DialogButton with defaults (blank label,
     *        no callback).
     */
    DialogButton() = default;

    /**
     * @brief Creates a @ref DialogButton with a label and optional callback.
     * @param label The button label string.
     * @param onClick Callback invoked on click.
     */
    DialogButton(const char* label, DialogButtonCallback onClick = nullptr);
  };
}
