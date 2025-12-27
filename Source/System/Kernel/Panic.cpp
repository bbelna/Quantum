/**
 * @file System/Kernel/Panic.cpp
 * @brief Panic handling utilities.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <CString.hpp>
#include <Debug.hpp>

#include "CPU.hpp"
#include "Logger.hpp"
#include "Panic.hpp"

namespace Quantum::System::Kernel {
  void Panic(
    CString message,
    CString file,
    UInt32 line,
    CString function
  ) {
    using namespace ::Quantum;

    using LogLevel = Logger::Level;

    CString fileStr = file ? file : "unknown";
    CString funcStr = function ? function : "unknown";
    CString lineStr = nullptr;
    char lineBuffer[16] = {};

    if (
      line > 0
      && ToCString(line, lineBuffer, sizeof(lineBuffer))
    ) {
      lineStr = lineBuffer;
    } else {
      lineStr = "unknown";
    }

    static char info[256];

    for (Size i = 0; i < sizeof(info); ++i) {
      info[i] = '\0';
    }

    Size out = 0;

    auto append = [&](CString src) -> bool {
      if (!src) {
        return true;
      } else {
        Size len = Length(src);

        if (out + len >= sizeof(info)) {
          return false;
        } else {
          for (Size i = 0; i < len; ++i) {
            info[out++] = src[i];
          }

          return true;
        }
      }
    };

    CString trimmedFile = TrimSourceFile(fileStr);

    append("  ");
    append(trimmedFile);
    append(":");
    append(lineStr);
    append(" (");
    append(funcStr);
    append(")");

    info[out] = '\0';

    char panicMessage[256] = {};

    Concat(
      "  ",
      message ? message : "unknown",
      panicMessage,
      sizeof(panicMessage)
    );

    Logger::Write(LogLevel::Panic, ":( PANIC");
    Logger::Write(LogLevel::Panic, panicMessage);
    Logger::Write(LogLevel::Panic, info);

    CPU::HaltForever();

    __builtin_unreachable();
  }
}
