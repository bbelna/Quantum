/**
 * @file Utilities/qsh/Shell.hpp
 * @brief Declares @ref @Q::Shell::Shell.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "ShellConstants.hpp"

namespace Quantum::Shell {
  /**
   * @brief The QuantumOS shell.
   *
   * In direct mode, acquires the display via @ref DisplayClient and
   * drives the screen in text mode, reading keyboard input from the
   * input server. In stream mode (when launched with inherited
   * standard streams), reads from @ref StandardIn and writes to
   * @ref StandardOut, allowing a parent @ref Process to bridge I/O.
   */
  class Shell {
    public:
      /**
       * @brief Creates a new @ref Shell.
       */
      Shell() = default;

      /**
       * @brief Initializes clients and enters the main shell loop.
       * @return Exit code (`0` on normal exit).
       */
      int Run();

    private:
      /**
       * @brief The @ref DisplayClient for text-mode rendering.
       */
      DisplayClient _displayClient;

      /**
       * @brief The @ref InputClient for keyboard events.
       */
      InputClient _inputClient;

      /**
       * @brief The @ref FileSystemClient for file system operations.
       */
      FileSystemClient _fileSystemClient;

      /**
       * @brief The @ref RunClient for launching executables.
       */
      RunClient _runClient;

      /**
       * @brief The @ref KernelClient for process management.
       */
      KernelClient _kernel;

      /**
       * @brief The @ref StreamClient for creating child output streams.
       */
      StreamClient _streamClient;

      /**
       * @brief Current volume label.
       */
      char _volume[MaxVolumeLabelLength];

      /**
       * @brief Current working directory relative to the volume. Root is
       *        represented as an empty string; subdirectories have a leading
       *        slash.
       */
      char _directory[MaxPathLength];

      /**
       * @brief Command line input buffer.
       */
      char _inputBuffer[MaxInputLength];

      /**
       * @brief Current position in the input buffer.
       */
      Size _inputPosition = 0;

      /**
       * @brief Whether the shell is running in stream mode (using
       *        @ref StandardIn / @ref StandardOut) rather than direct display
       *        mode.
       */
      bool _streamMode = false;

      /**
       * @brief Frozen absolute search directory loaded from
       *        `qsh.cfg` (`PATH` key) at startup, or an empty
       *        string if no valid `PATH` was configured.
       *
       * The path is captured at Shell startup by resolving the
       * configured value relative to the working directory at that
       * time, and is *not* updated when the working directory changes
       * later via `cd` / `cv`.
       */
      char _searchPath[FileSystemMaxPathLength] = {};

      /**
       * @brief Writes a null-terminated string to the output.
       * @note In stream mode, writes to @ref StandardOut. In direct mode,
       *       writes via @ref DisplayClient::WriteText.
       * @param text The text to write.
       */
      void _write(const char* text);

      /**
       * @brief Reads the next input character.
       * @param outCharacter Receives the ASCII character.
       * @param outScancode Receives the hardware scancode (`0` in stream
       *                    mode).
       * @return `true` if a character was read, `false` if the input
       *         stream was closed (stream mode only).
       * @note In stream mode, reads from @ref StandardIn via
       *       @ref Stream::ReadByte. In direct mode, polls @ref InputClient for
       *       a key-down event.
       */
      bool _readInput(
        char& outCharacter,
        UInt16& outScancode
      );

      /**
       * @brief Writes the shell prompt to the output.
       */
      void _writePrompt();

      /**
       * @brief Publishes the current volume and directory as this process's
       *        working directory on the run server.
       */
      void _syncWorkingDirectory();

      /**
       * @brief Builds a full file system path from the current volume and
       *        directory.
       * @param buffer Pointer to the destination `char` buffer.
       * @param bufferSize @ref Size of the buffer.
       */
      void _buildCurrentPath(
        char* buffer,
        Size bufferSize
      );

      /**
       * @brief Processes a completed command line.
       */
      void _executeCommand();

      /**
       * @brief Parses the input buffer into arguments.
       * @param arguments Array to receive argument pointers.
       * @param maxArguments Maximum number of arguments.
       * @return Number of arguments parsed.
       */
      Size _parseArguments(
        char** arguments,
        Size maxArguments
      );

      /**
       * @brief Handles the `cd` built-in command.
       * @param arguments The argument array.
       * @param argumentCount Number of arguments.
       */
      void _changeDirectory(
        char** arguments,
        Size argumentCount
      );

      /**
       * @brief Handles the `cv` built-in command.
       * @param arguments The argument array.
       * @param argumentCount Number of arguments.
       */
      void _changeVolume(
        char** arguments,
        Size argumentCount
      );

      /**
       * @brief Normalizes a relative path against the current directory.
       * @param relativePath The relative path to normalize.
       * @param outDirectory
       *   Pointer to the `char` buffer to receive the normalized directory
       *   (relative to volume root, with leading `/` for subdirectories, empty
       *   string for root).
       * @param outSize @ref Size of the output buffer.
       *
       * Processes `'.'` (skip), `'..'` (go up one level), strips leading
       * `'./'`, and handles compound paths like `'../../foo/bar'`.
       */
      void _normalizePath(
        const char* relativePath,
        char* outDirectory,
        Size outSize
      );

      /**
       * @brief Returns whether a command string contains a path
       *        separator (`'/'`).
       * @param command The command string to check.
       * @return `true` if the string contains `'/'`.
       */
      bool _hasPathComponent(const char* command);

      /**
       * @brief Returns whether a command string contains a file
       *        extension (`'.'`).
       * @param command The command string to check.
       * @return `true` if the string contains `'.'`.
       */
      bool _hasExtension(const char* command);

      /**
       * @brief Resolves a command name or relative path to an absolute
       *        file system path.
       * @note Commands with path components (contain `'/'`) are resolved
       *       relative to the current working directory.
       * @param command The command name or relative path.
       * @param outPath Pointer to a `char` buffer to receive the resolved path.
       * @param pathSize @ref Size of the output buffer.
       * @return `true` if a valid executable was found.
       */
      bool _resolvePath(
        const char* command,
        char* outPath,
        Size pathSize
      );

      /**
       * @brief Loads `qsh.cfg` from the spawn-time working directory
       *        (inherited from the parent process) or the program
       *        directory, and freezes @ref _searchPath from its
       *        `PATH` key.
       *
       * Silently does nothing if the file does not exist, fails to
       * open or read, contains no `PATH` key, has an empty `PATH`
       * value, or resolves to a directory that cannot be stat'd as
       * a directory.
       */
      void _loadConfiguration();

      /**
       * @brief Tests whether @p stat-able @p path exists and is a
       *        regular non-empty file, formatting the resolved path
       *        into @p outPath on success.
       * @param command Command name (without extension fallback).
       * @param directory
       *   Absolute directory (volume + path, no trailing slash) in
       *   which to look for @p command and `<command>.qx`.
       * @param outPath Receives the resolved absolute path on success.
       * @param pathSize Size of @p outPath in bytes.
       * @return `true` if a matching executable file was found.
       */
      bool _tryResolveInDirectory(
        const char* command,
        const char* directory,
        char* outPath,
        Size pathSize
      );

      /**
       * @brief Attempts to run a command.
       * @param command The command name or path.
       * @param arguments The full argument array.
       * @param argumentCount Number of arguments.
       */
      void _run(
        const char* command,
        char** arguments,
        Size argumentCount
      );
  };
}
