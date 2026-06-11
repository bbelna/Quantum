/**
 * @file Include/Quantum/Servers/Graphics/GraphicsServerABI.hpp
 * @brief Includes all graphics server ABI types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "GraphicsServerConstants.hpp"
#include "GraphicsServerOperation.hpp"
#include "GraphicsServerRequest.hpp"
#include "ABI/WriteTextRequest.hpp"
#include "ABI/SetModeRequest.hpp"
#include "ABI/FillRectangleRequest.hpp"
#include "ABI/BlitBufferRequest.hpp"
#include "ABI/XORRectangleRequest.hpp"
#include "ABI/SetCursorBitmapRequest.hpp"
#include "ABI/MoveCursorRequest.hpp"
#include "ABI/ShowCursorRequest.hpp"
#include "ABI/GetModeInfoRequest.hpp"
#include "ABI/GetModeInfoResult.hpp"
#include "ABI/ScreenBlitRequest.hpp"
#include "ABI/BeginBatchRequest.hpp"
#include "ABI/EndBatchRequest.hpp"
#include "ABI/DrawTextRequest.hpp"
#include "ABI/GetBackBufferRequest.hpp"
#include "ABI/GetBackBufferResult.hpp"
#include "ABI/FlushBackBufferRequest.hpp"
