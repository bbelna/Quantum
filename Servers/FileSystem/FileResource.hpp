/**
 * @file Servers/FileSystem/FileResource.hpp
 * @brief Declares @ref @QFSSrv::FileResource and
 *        @ref @QFSSrv::FileResourceRepository.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <FileSystemServerTypes.hpp>

namespace Quantum::Servers::FileSystem {
  /**
   * @brief Maximum number of simultaneously open file resources.
   */
  constexpr Size MaxFileResources = 256;

  /**
   * @brief A @ref Resource representing an open file in the file system
   *        server.
   *
   * The file system server acts as a proxy between clients and backend
   * file system implementation services. Each open file produces a
   * resource with a client-facing ID (the @ref Resource::ID, returned to
   * the calling process) and a backend handle (returned by the
   * implementation service). This struct maintains the mapping between
   * the two.
   */
  struct FileResource : public Resource {
    /**
     * @brief @ref FileHandle on the backend file system implementation
     *        service.
     */
    FileHandle BackendHandle;

    /**
     * @brief @ref IPCPortID of the backend service managing this file.
     */
    IPCPortID ServicePortID;

    /**
     * @brief @ref ProcessID of the client that opened this file.
     */
    ProcessID ClientPID;
  };

  /**
   * @brief Fixed-capacity repository for @ref FileResource instances.
   */
  using FileResourceRepository
    = ResourceRepository<FileResource, MaxFileResources>;
}
