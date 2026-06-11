/**
 * @file Servers/Startup/StartupServer.hpp
 * @brief Declares @ref @QStpSrv::StartupServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <StartupServerTypes.hpp>

namespace Quantum::Servers::Startup {
  /**
   * @brief Initializes QuantumOS during startup.
   *
   * Spawns all servers, drivers, and applications from the
   * @ref StartupBundle, waiting for required entries to signal
   * readiness before proceeding to the next phase. The server exits
   * after the boot sequence completes.
   */
  class StartupServer : public Server {
    public:
      /**
       * @brief Creates a new @ref StartupServer.
       * @param log Reference to the @ref ServerLog for logging. Must
       *            outlive this server instance.
       * @param bundle The startup bundle containing initialization data
       *               and resources. Must outlive this instance.
       */
      StartupServer(
        ServerLog& log,
        StartupBundle* bundle
      );

      /**
       * @brief Runs the boot sequence and returns when complete.
       * @return `0` on success, non-zero on failure.
       */
      Int32 Start();

    private:
      /**
       * @brief Kernel client for this server's direct kernel operations.
       *
       * Declared first so later members are initialized after it.
       */
      KernelClient _kernel;

      /**
       * @brief Reference to the server log.
       */
      ServerLog& _log;

      /**
       * @brief The startup bundle containing initialization data and resources.
       */
      StartupBundle* _bundle;

      /**
       * @brief Spawns all raw format system entries from the startup bundle.
       */
      void _spawnRawSystemEntries();

      /**
       * @brief Spawns ELF format entries from the startup bundle of the
       *        specified type.
       * @param type The type of entries to spawn.
       */
      void _spawnELF(StartupBundleEntryType type);

      /**
       * @brief Logs the spawning of a server entry.
       * @param entry The startup bundle entry being spawned.
       */
      void _logSpawn(const StartupBundleEntry& entry);

      /**
       * @brief Reads ELF binaries from the QUANTUM volume for all bundle
       *        entries of the specified type with DiskELF format, and spawns
       *        them via the Run server. Blocks until entries with the Required
       *        flag signal startup completion.
       * @param type The type of entries to spawn.
       */
      void _spawnFromDisk(StartupBundleEntryType type);
  };
}
