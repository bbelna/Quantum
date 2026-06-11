/**
 * @file Utilities/qsh/Shell.cpp
 * @brief Implements @ref @Q::Shell::Shell.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "Shell.hpp"

namespace Quantum::Shell {
  int Shell::Run() {
    _streamMode = StandardOut.IsValid();

    if (!_streamMode) {
      _kernel.SetLogLevel(LogLevel::Warning);

      if (
        !_displayClient.Initialize() ||
        !_displayClient.AcquireDisplay()
      ) {
        return 1;
      } else if (!_inputClient.Initialize()) {
        _displayClient.ReleaseDisplay();

        return 1;
      }
    }

    // detect the boot volume
    FileSystemVolumeInfo volumes[1];

    if (
      _fileSystemClient.ListVolumes(
        volumes,
        1
      ) > 0
    ) {
      CString::Copy(
        volumes[0].Label,
        _volume,
        sizeof(_volume)
      );
    } else {
      CString::Copy(
        "?",
        _volume,
        sizeof(_volume)
      );
    }

    _directory[0] = '\0';
    _inputPosition = 0;
    _inputBuffer[0] = '\0';
    _searchPath[0] = '\0';

    _loadConfiguration();
    _syncWorkingDirectory();
    _writePrompt();

    // main input loop
    char character = 0;
    UInt16 scancode = 0;

    for (;;) {
      if (
        _readInput(
          character,
          scancode
        )
      ) {
        if (character == '\n') { // enter
          _write("\n");

          _inputBuffer[_inputPosition] = '\0';

          if (_inputPosition > 0) _executeCommand();

          _inputPosition = 0;
          _inputBuffer[0] = '\0';

          _writePrompt();

          continue;
        } else if ( // backspace
          character == '\b' ||
          scancode == 0x0E
        ) {
          if (_inputPosition > 0) {
            --_inputPosition;
            _inputBuffer[_inputPosition] = '\0';

            _write("\b \b");
          }

          continue;
        } else if ( // printable characters
          character >= ' ' &&
          character <= '~'
        ) {
          if (_inputPosition < MaxInputLength - 1) {
            _inputBuffer[_inputPosition++] = character;
            _inputBuffer[_inputPosition] = '\0';

            char echo[2] = { character, '\0' };

            _write(echo);
          }

          continue;
        }
      } else if (_streamMode) {
        // stdin was closed by the parent process - exit gracefully
        break;
      }
    }

    if (!_streamMode) {
      _displayClient.ReleaseDisplay();
    }

    return 0;
  }

  void Shell::_write(const char* text) {
    if (_streamMode) {
      StandardOut.Write(text);
    } else {
      _displayClient.WriteText(text);
    }
  }

  bool Shell::_readInput(
    char& outCharacter,
    UInt16& outScancode
  ) {
    if (_streamMode) {
      UInt8 byte = 0;

      if (!StandardIn.ReadByte(byte)) {
        return false;
      } else {
        outCharacter = static_cast<char>(byte);
        outScancode = 0;

        return true;
      }
    } else {
      InputEvent event;

      for (;;) {
        if (
          !_inputClient.GetNextEvent(&event) ||
          event.Type != InputEventType::KeyDown
        ) {
          continue;
        } else {
          outCharacter = event.Character;
          outScancode = event.Scancode;

          return true;
        }
      }
    }
  }

  void Shell::_writePrompt() {
    // char prompt[
    //   FileSystemMaxVolumeLabelLength +
    //   FileSystemMaxPathLength +
    //   8
    // ];

    // if (_directory[0] == '\0') {
    //   CString::Format(
    //     prompt,
    //     sizeof(prompt),
    //     "%s $ ",
    //     _volume
    //   );
    // } else {
    //   CString::Format(
    //     prompt,
    //     sizeof(prompt),
    //     "%s%s $ ",
    //     _volume,
    //     _directory
    //   );
    // }

    _write("$ ");
  }

  void Shell::_buildCurrentPath(char* buffer, Size bufferSize) {
    CString::Format(
      buffer,
      bufferSize,
      "%s%s",
      _volume,
      _directory
    );
  }

  Size Shell::_parseArguments(
    char** arguments,
    Size maxArguments
  ) {
    Size count = 0;
    char* cursor = _inputBuffer;

    while (
      *cursor != '\0' &&
      count < maxArguments
    ) {
      // skip leading spaces
      while (*cursor == ' ') {
        ++cursor;
      }

      if (*cursor == '\0') {
        break;
      }

      arguments[count++] = cursor;

      // find end of argument
      while (*cursor != '\0' && *cursor != ' ') {
        ++cursor;
      }

      if (*cursor == ' ') {
        *cursor = '\0';
        ++cursor;
      }
    }

    return count;
  }

  void Shell::_executeCommand() {
    char* arguments[MaxArguments];
    Size argumentCount = _parseArguments(
      arguments,
      MaxArguments
    );

    if (argumentCount > 0) {
      const char* command = arguments[0];

      if (
        CString::Equals(
          command,
          "cd"
        )
      ) {
        _changeDirectory(
          arguments,
          argumentCount
        );
      } else if (
        CString::Equals(
          command,
          "cv"
        )
      ) {
        _changeVolume(
          arguments,
          argumentCount
        );
      } else if (
        CString::Equals(
          command,
          "exit"
        )
      ) {
        if (!_streamMode) {
          _displayClient.ReleaseDisplay();
        }

        _kernel.ExitProcess(0);
      } else {
        _run(
          command,
          arguments,
          argumentCount
        );
      }
    }
  }

  void Shell::_normalizePath(
    const char* relativePath,
    char* outDirectory,
    Size outSize
  ) {
    // start from the current directory
    CString::Copy(
      _directory,
      outDirectory,
      outSize
    );

    const char* cursor = relativePath;

    // strip leading "./"
    if (
      cursor[0] == '.' &&
      cursor[1] == '/'
    ) {
      cursor += 2;
    }

    // process each path component separated by '/'
    while (*cursor != '\0') {
      // skip consecutive slashes
      while (*cursor == '/') {
        ++cursor;
      }

      if (*cursor == '\0') {
        break;
      }

      // find end of this component
      const char* componentStart = cursor;
      Size componentLength = 0;

      while (
        cursor[componentLength] != '\0' &&
        cursor[componentLength] != '/'
      ) {
        ++componentLength;
      }

      cursor += componentLength;

      // skip "." (current directory)
      if (componentLength == 1 && componentStart[0] == '.') {
        continue;
      }

      // handle ".." (go up one level)
      if (
        componentLength == 2 &&
        componentStart[0] == '.' &&
        componentStart[1] == '.'
      ) {
        Size length = CString::Length(outDirectory);

        if (length == 0) continue;

        // find and remove the last component
        for (
          Size index = length - 1;
          index > 0;
          --index
        ) {
          if (outDirectory[index] == '/') {
            outDirectory[index] = '\0';

            break;
          }

          // reached the beginning without finding a slash -
          // only one component remains, go to root
          if (index == 1) {
            outDirectory[0] = '\0';
          }
        }

        // single-char directory like "/x" - the loop above won't
        // clear it since index 0 is skipped
        if (CString::Length(outDirectory) == 1) {
          outDirectory[0] = '\0';
        }

        continue;
      }

      // regular component: append "/<component>"
      Size currentLength = CString::Length(outDirectory);

      if (currentLength + 1 + componentLength >= outSize - 1) {
        break;
      }

      outDirectory[currentLength] = '/';

      for (
        Size index = 0;
        index < componentLength;
        ++index
      ) {
        outDirectory[currentLength + 1 + index] = componentStart[index];
      }

      outDirectory[currentLength + 1 + componentLength] = '\0';
    }
  }

  void Shell::_changeDirectory(
    char** arguments,
    Size argumentCount
  ) {
    if (argumentCount < 2) {
      _write("Usage: cd <directory>\n");

      return;
    }

    const char* target = arguments[1];

    // cd / - go to root
    if (
      CString::Equals(
        target,
        "/"
      ) ||
      CString::Equals(
        target,
        "\\"
      )
    ) {
      _directory[0] = '\0';

      _syncWorkingDirectory();

      return;
    }

    // normalize the path relative to the current directory
    char newDirectory[FileSystemMaxPathLength];

    _normalizePath(
      target,
      newDirectory,
      sizeof(newDirectory)
    );

    // validate that the target is a directory
    char fullPath[FileSystemMaxPathLength];

    CString::Format(
      fullPath,
      sizeof(fullPath),
      "%s%s",
      _volume,
      newDirectory
    );

    FileSystemFileStat fileStat = {};

    if (
      !_fileSystemClient.Stat(
        fullPath,
        &fileStat
      ) ||
      fileStat.Type != FileSystemEntryType::Directory
    ) {
      char message[320];

      CString::Format(
        message,
        sizeof(message),
        "\"%s\" is not a directory\n",
        target
      );

      _write(message);

      return;
    }

    CString::Copy(
      newDirectory,
      _directory,
      sizeof(_directory)
    );

    _syncWorkingDirectory();
  }

  void Shell::_changeVolume(
    char** arguments,
    Size argumentCount
  ) {
    if (argumentCount < 2) {
      _write("Usage: cv <volume>\n");

      return;
    }

    // verify the volume exists
    FileSystemVolumeInfo volumes[8];
    Int32 count = _fileSystemClient.ListVolumes(
      volumes,
      8
    );

    for (
      Int32 index = 0;
      index < count;
      ++index
    ) {
      if (
        CString::Equals(
          volumes[index].Label,
          arguments[1]
        ) ||
        CString::Equals(
          volumes[index].VolumeID,
          arguments[1]
        )
      ) {
        CString::Copy(
          volumes[index].Label,
          _volume,
          sizeof(_volume)
        );

        _directory[0] = '\0';

        _syncWorkingDirectory();

        return;
      }
    }

    char message[128];

    CString::Format(
      message,
      sizeof(message),
      "Volume \"%s\" not found\n",
      arguments[1]
    );

    _write(message);
  }

  bool Shell::_hasPathComponent(const char* command) {
    for (
      Size index = 0;
      command[index] != '\0';
      ++index
    ) {
      if (command[index] == '/') {
        return true;
      }
    }

    return false;
  }

  bool Shell::_hasExtension(const char* command) {
    for (
      Size index = 0;
      command[index] != '\0';
      ++index
    ) {
      if (command[index] == '.') {
        return true;
      }
    }

    return false;
  }

  bool Shell::_tryResolveInDirectory(
    const char* command,
    const char* directory,
    char* outPath,
    Size pathSize
  ) {
    FileSystemFileStat fileStat = {};
    bool hasExtension = _hasExtension(command);

    // try <directory>/<command> as-is
    CString::Format(
      outPath,
      pathSize,
      "%s/%s",
      directory,
      command
    );

    if (
      _fileSystemClient.Stat(
        outPath,
        &fileStat
      ) &&
      fileStat.Type == FileSystemEntryType::Regular &&
      fileStat.Size > 0
    ) {
      return true;
    }

    // if no extension was given, also try <directory>/<command>.qx
    if (!hasExtension) {
      CString::Format(
        outPath,
        pathSize,
        "%s/%s.qx",
        directory,
        command
      );

      if (
        _fileSystemClient.Stat(
          outPath,
          &fileStat
        ) &&
        fileStat.Type == FileSystemEntryType::Regular &&
        fileStat.Size > 0
      ) {
        return true;
      }
    }

    return false;
  }

  bool Shell::_resolvePath(
    const char* command,
    char* outPath,
    Size pathSize
  ) {
    bool hasPath = _hasPathComponent(command);

    // if the command has path components (e.g. ./Apps/Terminal.qapp or
    // Apps/Terminal.qapp), resolve relative to the current directory
    if (hasPath) {
      FileSystemFileStat fileStat = {};
      const char* resolved = command;

      // strip leading "./"
      if (
        command[0] == '.' &&
        command[1] == '/'
      ) {
        resolved = command + 2;
      }

      if (_directory[0] == '\0') {
        CString::Format(
          outPath,
          pathSize,
          "%s/%s",
          _volume,
          resolved
        );
      } else {
        CString::Format(
          outPath,
          pathSize,
          "%s%s/%s",
          _volume,
          _directory,
          resolved
        );
      }

      if (
        _fileSystemClient.Stat(
          outPath,
          &fileStat
        ) &&
        fileStat.Type == FileSystemEntryType::Regular &&
        fileStat.Size > 0
      ) {
        return true;
      }

      return false;
    }

    // bare command name - first consult the frozen PATH from
    // qsh.cfg, if one was configured
    if (_searchPath[0] != '\0') {
      if (
        _tryResolveInDirectory(
          command,
          _searchPath,
          outPath,
          pathSize
        )
      ) {
        return true;
      }
    }

    // fall back to current directory
    char currentDirectory[FileSystemMaxPathLength];

    _buildCurrentPath(
      currentDirectory,
      sizeof(currentDirectory)
    );

    return _tryResolveInDirectory(
      command,
      currentDirectory,
      outPath,
      pathSize
    );
  }

  void Shell::_run(
    const char* command,
    char** arguments,
    Size argumentCount
  ) {
    static constexpr Size maxChildArguments = 32;
    char path[FileSystemMaxPathLength];

    if (
      !_resolvePath(
        command,
        path,
        sizeof(path)
      )
    ) {
      char message[320];

      CString::Format(
        message,
        sizeof(message),
        "\"%s\" not found\n",
        command
      );

      _write(message);

      return;
    }

    // read the ELF binary
    FileHandle fileHandle = _fileSystemClient.Open(
      path,
      Enum::ToBase(FileSystemOpenFlags::Read)
    );

    if (fileHandle == 0) {
      _write("Failed to open executable\n");

      return;
    }

    FileSystemFileStat fileStat = {};

    _fileSystemClient.Stat(
      path,
      &fileStat
    );

    UIntPtr address = _kernel.AllocateBlock(fileStat.Size);

    if (address == 0) {
      _fileSystemClient.Close(fileHandle);

      _write("Out of memory\n");

      return;
    }

    Int32 bytesRead = _fileSystemClient.Read(
      fileHandle,
      reinterpret_cast<void*>(address),
      fileStat.Size,
      0
    );

    _fileSystemClient.Close(fileHandle);

    if (bytesRead <= 0) {
      _kernel.FreeBlock(address);

      _write("Failed to read executable\n");

      return;
    }

    // create a stream for the child's stdout/stderr
    SharedBufferID stdoutBufferID = 0;
    UIntPtr streamBufferAddress = 0;
    StreamHeader* streamHeader = nullptr;
    bool hasStream = _streamClient.CreateStream(stdoutBufferID);

    if (hasStream) {
      streamBufferAddress = _kernel.AttachSharedBuffer(stdoutBufferID);

      if (streamBufferAddress == 0) {
        hasStream = false;

        _streamClient.CloseReader(stdoutBufferID);
      } else {
        streamHeader = reinterpret_cast<StreamHeader*>(streamBufferAddress);
      }
    }

    // when running in stream mode, also create a stdin stream so the
    // child can read input forwarded from our own StandardIn
    SharedBufferID stdinBufferID = 0;
    Stream childStdin;
    bool hasStdin = false;

    if (_streamMode && hasStream) {
      if (_streamClient.CreateStream(stdinBufferID)) {
        childStdin = Stream::Open(stdinBufferID);

        if (childStdin.IsValid()) {
          hasStdin = true;
        } else {
          _streamClient.CloseReader(stdinBufferID);

          stdinBufferID = 0;
        }
      }
    }

    const char* childArguments[maxChildArguments];
    Size childArgumentCount = 0;

    childArguments[childArgumentCount++] = command;

    // pass through additional arguments (skip argv[0] which is the command)
    for (Size index = 1; index < argumentCount; ++index) {
      if (childArgumentCount >= maxChildArguments) {
        break;
      }

      childArguments[childArgumentCount++] = arguments[index];
    }

    // prepare inherited stream buffer IDs
    UInt8 streamCount = 0;
    SharedBufferID streamBufferIDs[3] = {};

    if (hasStream) {
      streamCount = 3;
      streamBufferIDs[0] = hasStdin ? stdinBufferID : 0;
      streamBufferIDs[1] = stdoutBufferID;
      streamBufferIDs[2] = stdoutBufferID;
    }

    char currentWorkingDirectory[FileSystemMaxPathLength];

    _buildCurrentPath(
      currentWorkingDirectory,
      sizeof(currentWorkingDirectory)
    );

    ProcessID pid = _runClient.LoadELF(
      path,
      currentWorkingDirectory,
      reinterpret_cast<const void*>(address),
      static_cast<Size>(bytesRead),
      childArgumentCount,
      childArguments,
      streamCount,
      streamBufferIDs
    );

    _kernel.FreeBlock(address);

    if (pid == InvalidProcessID) {
      char message[320];

      CString::Format(
        message,
        sizeof(message),
        "Failed to launch \"%s\"\n",
        command
      );

      _write(message);

      if (hasStdin) {
        childStdin.Close();
      }

      if (hasStream) {
        _kernel.DetachSharedBuffer(streamBufferAddress);
        _streamClient.CloseReader(stdoutBufferID);
      }

      return;
    }

    // poll for child output until the process exits
    if (hasStream) {
      char pollBuffer[512];

      while (
        _kernel.IsProcessAlive(pid) ||
        streamHeader->Available() > 0
      ) {
        // forward our stdin to the child's stdin
        if (hasStdin) {
          char stdinBuffer[128];
          Size stdinBytes = StandardIn.Read(
            stdinBuffer,
            sizeof(stdinBuffer)
          );

          if (stdinBytes > 0) {
            childStdin.Write(stdinBuffer, stdinBytes);
          }
        }

        UInt32 available = streamHeader->Available();

        if (available == 0) {
          _kernel.SleepThread(10);

          continue;
        }

        if (available > sizeof(pollBuffer) - 1) {
          available = sizeof(pollBuffer) - 1;
        }

        const UInt8* ringData = streamHeader->Data();
        UInt32 readOffset = streamHeader->ReadOffset;
        UInt32 capacity = streamHeader->Capacity;

        for (
          UInt32 byteIndex = 0;
          byteIndex < available;
          ++byteIndex
        ) {
          pollBuffer[byteIndex] = static_cast<char>(
            ringData[(readOffset + byteIndex) % capacity]
          );
        }

        pollBuffer[available] = '\0';

        streamHeader->ReadOffset = (readOffset + available) % capacity;

        _write(pollBuffer);
      }

      if (hasStdin) {
        childStdin.Close();
      }

      _kernel.DetachSharedBuffer(streamBufferAddress);
      _streamClient.CloseReader(stdoutBufferID);
    } else {
      // no stream available - just wait for the process to exit
      _kernel.WaitForProcess(pid);
    }
  }

  void Shell::_syncWorkingDirectory() {
    char currentPath[FileSystemMaxPathLength];

    _buildCurrentPath(
      currentPath,
      sizeof(currentPath)
    );

    _runClient.SetWorkingDirectory(
      _kernel.GetProcessID(),
      currentPath
    );
  }

  void Shell::_loadConfiguration() {
    _searchPath[0] = '\0';

    // try to find the config file: first in the spawn-time working
    // directory (inherited from the parent), then fall back to the
    // program directory (where qsh.qx lives)
    char configPath[FileSystemMaxPathLength];
    char spawnDirectory[FileSystemMaxPathLength];

    if (
      _runClient.GetWorkingDirectory(
        _kernel.GetProcessID(),
        spawnDirectory,
        sizeof(spawnDirectory)
      )
    ) {
      CString::Format(
        configPath,
        sizeof(configPath),
        "%s/%s",
        spawnDirectory,
        ConfigurationFileName
      );
    } else {
      configPath[0] = '\0';
    }

    FileSystemFileStat fileStat = {};

    if (
      configPath[0] == '\0' ||
      !(
        _fileSystemClient.Stat(
          configPath,
          &fileStat
        ) &&
        fileStat.Type == FileSystemEntryType::Regular &&
        fileStat.Size > 0
      )
    ) {
      char programDirectory[FileSystemMaxPathLength];

      _runClient.GetProgramDirectory(
        _kernel.GetProcessID(),
        programDirectory,
        sizeof(programDirectory)
      );

      CString::Format(
        configPath,
        sizeof(configPath),
        "%s/%s",
        programDirectory,
        ConfigurationFileName
      );
    }

    fileStat = {};

    if (
      _fileSystemClient.Stat(
        configPath,
        &fileStat
      ) &&
      fileStat.Type == FileSystemEntryType::Regular &&
      fileStat.Size > 0
    ) {
      Size readLength = fileStat.Size;

      if (readLength >= MaxConfigurationFileSize) {
        readLength = MaxConfigurationFileSize - 1;
      }

      char configBuffer[MaxConfigurationFileSize];

      FileHandle fileHandle = _fileSystemClient.Open(
        configPath,
        Enum::ToBase(FileSystemOpenFlags::Read)
      );

      if (fileHandle != 0) {
        Int32 bytesRead = _fileSystemClient.Read(
          fileHandle,
          configBuffer,
          static_cast<UInt32>(readLength),
          0
        );

        _fileSystemClient.Close(fileHandle);

        if (bytesRead > 0) {
          configBuffer[bytesRead] = '\0';

          ConfigurationParser parser;

          parser.Parse(
            configBuffer,
            static_cast<Size>(bytesRead)
          );

          const char* pathValue = parser.Find(ConfigurationPathKey);

          if (
            pathValue != nullptr &&
            pathValue[0] != '\0'
          ) {
            // resolve PATH relative to the current working directory
            // and freeze the absolute result
            char candidate[FileSystemMaxPathLength];

            // an absolute PATH (starting with '/') is rooted at the
            // volume; otherwise it is appended to the current cwd
            if (pathValue[0] == '/') {
              CString::Format(
                candidate,
                sizeof(candidate),
                "%s%s",
                _volume,
                pathValue
              );
            } else if (_directory[0] == '\0') {
              CString::Format(
                candidate,
                sizeof(candidate),
                "%s/%s",
                _volume,
                pathValue
              );
            } else {
              CString::Format(
                candidate,
                sizeof(candidate),
                "%s%s/%s",
                _volume,
                _directory,
                pathValue
              );
            }

            // strip a single trailing slash so that PATH-resolved
            // children format cleanly as "<path>/<command>"
            Size candidateLength = CString::Length(candidate);

            if (
              candidateLength > 0 &&
              candidate[candidateLength - 1] == '/'
            ) {
              candidate[candidateLength - 1] = '\0';
            }

            // validate that the resolved path is a real directory
            FileSystemFileStat pathStat = {};

            if (
              _fileSystemClient.Stat(
                candidate,
                &pathStat
              ) &&
              pathStat.Type == FileSystemEntryType::Directory
            ) {
              CString::Copy(
                candidate,
                _searchPath,
                sizeof(_searchPath)
              );
            }
          }
        }
      }
    }

    // if no PATH was configured, default to the program directory
    // (the directory from which qsh.qx was loaded)
    if (_searchPath[0] == '\0') {
      _runClient.GetProgramDirectory(
        _kernel.GetProcessID(),
        _searchPath,
        sizeof(_searchPath)
      );
    }
  }
}
