/**
 * @file Services/Drivers/Input/PS2/Controller/Controller.cpp
 * @brief PS/2 controller helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/IO.hpp>

#include "Controller.hpp"

namespace Quantum::Services::Drivers::Input::PS2 {
  using ABI::IO;

  bool Controller::WaitForRead() {
    const UInt32 maxSpins = 100000;

    for (UInt32 i = 0; i < maxSpins; ++i) {
      UInt8 status = IO::In8(_statusPort);

      if ((status & _statusOutputFull) != 0) {
        return true;
      }

    }

    return false;
  }

  bool Controller::WaitForWrite() {
    const UInt32 maxSpins = 100000;

    for (UInt32 i = 0; i < maxSpins; ++i) {
      UInt8 status = IO::In8(_statusPort);

      if ((status & _statusInputFull) == 0) {
        return true;
      }

    }

    return false;
  }

  bool Controller::Initialize() {
    // drain any pending output so subsequent reads are clean
    if ((IO::In8(_statusPort) & _statusOutputFull) != 0) {
      (void)IO::In8(_dataPort);
    }

    return WaitForWrite();
  }

  UInt8 Controller::ReadData() {
    if (!WaitForRead()) {
      return 0;
    }

    return IO::In8(_dataPort);
  }

  bool Controller::WriteCommand(UInt8 command) {
    if (!WaitForWrite()) {
      return false;
    }

    IO::Out8(_commandPort, command);

    return true;
  }

  bool Controller::WriteData(UInt8 value) {
    if (!WaitForWrite()) {
      return false;
    }

    IO::Out8(_dataPort, value);

    return true;
  }
}
