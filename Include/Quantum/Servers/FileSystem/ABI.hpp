/**
 * @file Include/Quantum/Servers/FileSystem/ABI.hpp
 * @brief Includes all file system server ABI types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "FileSystemConstants.hpp"
#include "FileSystemOperation.hpp"
#include "FileSystemError.hpp"
#include "FileSystemResult.hpp"
#include "FileHandle.hpp"
#include "FileSystemOpenFlags.hpp"
#include "FileSystemEntryType.hpp"
#include "FileSystemFileStat.hpp"
#include "FileSystemDirectory.hpp"
#include "FileSystemOpenRequest.hpp"
#include "FileSystemOpenResult.hpp"
#include "FileSystemCloseRequest.hpp"
#include "FileSystemReadRequest.hpp"
#include "FileSystemReadResult.hpp"
#include "FileSystemRequest.hpp"
#include "FileSystemRequestWithReply.hpp"
#include "FileSystemWriteRequest.hpp"
#include "FileSystemWriteResult.hpp"
#include "FileSystemStatRequest.hpp"
#include "FileSystemStatResult.hpp"
#include "FileSystemReadDirectoryRequest.hpp"
#include "FileSystemReadDirectoryResult.hpp"
#include "FileSystemDeleteRequest.hpp"
#include "FileSystemCreateDirectoryRequest.hpp"
#include "FileSystemVolumeInfo.hpp"
#include "FileSystemMountRequest.hpp"
#include "FileSystemUnmountRequest.hpp"
#include "FileSystemListVolumesRequest.hpp"
#include "FileSystemListVolumesResult.hpp"
