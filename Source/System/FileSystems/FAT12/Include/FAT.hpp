/**
 * @file System/FileSystems/FAT12/Include/FAT.hpp
 * @brief FAT12 file system FAT table helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::FileSystems::FAT12 {
  class Volume;

  /**
   * FAT table access helpers.
   */
  class FAT {
    public:
      /**
       * Initializes with a volume.
       * @param volume
       *   Volume to access.
       */
      void Initialize(Volume& volume);

      /**
       * Loads the FAT into cache if possible.
       * @return
       *   True if the cache was loaded.
       */
      bool LoadCache();

      /**
       * Reads a FAT entry (cluster link).
       * @param cluster
       *   Cluster to read.
       * @param nextCluster
       *   Receives the linked cluster.
       * @return
       *   True on success.
       */
      bool ReadEntry(UInt32 cluster, UInt32& nextCluster);

      /**
       * Reads a FAT entry from the cache.
       * @param cluster
       *   Cluster to read.
       * @param nextCluster
       *   Receives the linked cluster.
       * @return
       *   True on success.
       */
      bool ReadEntryCached(UInt32 cluster, UInt32& nextCluster) const;

      /**
       * Writes a FAT entry.
       * @param cluster
       *   Cluster to write.
       * @param value
       *   FAT value to store.
       * @return
       *   True on success.
       */
      bool WriteEntry(UInt32 cluster, UInt32 value);

      /**
       * Finds a free cluster.
       * @param outCluster
       *   Receives the free cluster index.
       * @return
       *   True on success.
       */
      bool FindFreeCluster(UInt32& outCluster);

      /**
       * Counts free clusters.
       * @param outCount
       *   Receives the free cluster count.
       * @return
       *   True on success.
       */
      bool CountFreeClusters(UInt32& outCount);

      /**
       * Frees a cluster chain.
       * @param startCluster
       *   First cluster in the chain.
       * @return
       *   True on success.
       */
      bool FreeClusterChain(UInt32 startCluster);

      /**
       * Checks if a FAT value denotes end of chain.
       * @param value
       *   FAT entry value.
       * @return
       *   True if end-of-chain.
       */
      static bool IsEndOfChain(UInt32 value);

    private:
      /**
       * Associated volume.
       */
      Volume* _volume;
  };
}
