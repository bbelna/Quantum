/**
 * @file Include/Quantum/Streaming.hpp
 * @brief Includes all headers for, and declares, @ref Quantum::Streaming.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Streaming/StreamDescriptor.hpp"
#include "Streaming/StreamHeader.hpp"
#include "Streaming/ProcessStreamTable.hpp"
#include "Streaming/StandardStreams.hpp"
#include "Streaming/Stream.hpp"

/**
 * @brief OS-independent shared-memory ring buffer primitives for
 *        inter-process stream communication.
 */
namespace Quantum::Streaming {}
