/**
 * @file Include/Quantum/Servers/FileSystem/Core/FileSystemError.hpp
 * @brief Declares @ref @QFSAbi::FileSystemError.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Error codes returned by file system server operations.
   */
  enum class FileSystemError : UInt32 {
    /**
     * @brief Operation completed successfully.
     */
    None = 0,

    // Volume management errors (1-9)

    /**
     * @brief The volume label exceeds the maximum allowed length.
     */
    LabelTooLong = 1,

    /**
     * @brief A volume with the specified label is already mounted.
     */
    AlreadyMounted = 2,

    /**
     * @brief The volume table is full; no more volumes can be mounted.
     */
    TableFull = 3,

    /**
     * @brief No volume with the specified label was found.
     */
    NotFound = 4,

    /**
     * @brief The path prefix matched multiple volumes by label. The caller
     *        must disambiguate by using the Volume ID instead.
     */
    AmbiguousVolume = 5,

    // File operation errors (10+)

    /**
     * @brief The path is malformed or empty.
     */
    InvalidPath = 10,

    /**
     * @brief The volume label in the path does not match any mounted volume.
     */
    VolumeNotFound = 11,

    /**
     * @brief The specified file or directory does not exist.
     */
    FileNotFound = 12,

    /**
     * @brief A file or directory with the specified name already exists.
     */
    AlreadyExists = 13,

    /**
     * @brief The caller does not have permission to perform the operation.
     */
    AccessDenied = 14,

    /**
     * @brief The specified file handle is invalid or does not belong to the
     *        calling process.
     */
    InvalidHandle = 15,

    /**
     * @brief The server's file handle table is full.
     */
    HandleTableFull = 16,

    /**
     * @brief The path refers to a non-directory where a directory was
     *        expected.
     */
    NotADirectory = 17,

    /**
     * @brief The path refers to a directory where a regular file was
     *        expected.
     */
    IsADirectory = 18,

    /**
     * @brief An I/O error occurred in the backend file system service.
     */
    IOError = 19,

    /**
     * @brief The volume has no remaining space.
     */
    NoSpace = 20,

    /**
     * @brief The requested operation is not supported.
     */
    InvalidOperation = 21,

    /**
     * @brief The backend file system service is unavailable.
     */
    ServiceUnavailable = 22
  };
}
