/**
 * @file Include/Quantum/HAL/Graphics/Payloads.hpp
 * @brief Aggregates all graphics driver operation payload structures.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Payloads/SetModePayload.hpp"
#include "Payloads/FillRectanglePayload.hpp"
#include "Payloads/BlitBufferPayload.hpp"
#include "Payloads/XORRectanglePayload.hpp"
#include "Payloads/ModeInfoPayload.hpp"
#include "Payloads/SetBatchModePayload.hpp"
#include "Payloads/FlushRegionPayload.hpp"
#include "Payloads/ScreenBlitPayload.hpp"
#include "Payloads/HardwareCursorSupportPayload.hpp"
#include "Payloads/SetHardwareCursorBitmapPayload.hpp"
#include "Payloads/SetHardwareCursorPositionPayload.hpp"
#include "Payloads/SetHardwareCursorVisiblePayload.hpp"
#include "Payloads/WritePixelRegionPayload.hpp"
#include "Payloads/FastScreenBlitSupportPayload.hpp"
#include "Payloads/WriteNativeRegionPayload.hpp"
#include "Payloads/FramebufferBufferIDPayload.hpp"
#include "Payloads/AcquireDisplayPayload.hpp"
#include "Payloads/ReleaseDisplayPayload.hpp"
#include "Payloads/DisplayOwnerPayload.hpp"

/**
 * @brief Operation-specific payload structures for the graphics device driver
 *        interface.
 *
 * Each struct in this namespace corresponds to a `GraphicsDriverOperation`
 * code and carries the typed arguments for that operation through the generic
 * `IDriver::Invoke` dispatch path.
 */
namespace Quantum::HAL::Graphics::Payloads {}
