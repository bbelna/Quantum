/**
 * @file Include/Quantum/HAL/Graphics.hpp
 * @brief Aggregates the graphics device driver interface and all associated
 *        payload structures.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Graphics/Payloads.hpp"
#include "Graphics/GraphicsDriver.hpp"

/**
 * @brief Graphics device interfaces.
 *
 * This namespace provides the abstract `GraphicsDriver` interface,
 * the `GraphicsDriverOperation` dispatch codes, and the
 * `Quantum::HAL::Graphics::Payloads` sub-namespace containing the
 * operation-specific payload structures used with
 * `IDriver::Invoke`.
 */
namespace Quantum::HAL::Graphics {}
