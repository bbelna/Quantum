/**
 * @file Applications/Diagnostics/TestSuite/Tests/FileSystemTests.cpp
 * @brief File system timeout tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/FileSystem.hpp>

#include "Testing.hpp"
#include "Tests/FileSystemTests.hpp"

namespace Quantum::Applications::Diagnostics::TestSuite::Tests {
  using ABI::FileSystem;

  bool FileSystemTests::TestRequestTimeout() {
    constexpr CString missingLabel = "__NO_SUCH_VOLUME__";
    FileSystem::VolumeHandle handle
      = FileSystem::OpenVolume(missingLabel, 1);

    if (handle != 0) {
      FileSystem::CloseVolume(handle);
    }

    bool ok = handle == 0;

    TEST_ASSERT(ok, "filesystem timeout override failed");

    return ok;
  }

  void FileSystemTests::RegisterTests() {
    Testing::Register("FileSystem timeout override", TestRequestTimeout);
  }
}
