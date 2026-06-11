/**
 * @file Include/Quantum/Threading.hpp
 * @brief Includes all threading-related headers.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Threading/OS/IOSProcess.hpp"
#include "Threading/OS/IOSThread.hpp"
#include "Threading/Process.hpp"
#include "Threading/Thread.hpp"

/**
 * @brief Quantum's thread and process library.
 */
namespace Quantum::Threading {}

namespace QThreading = Quantum::Threading;
