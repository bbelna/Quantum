/**
 * @file Runtime/QuantumCRT0.cpp
 * @brief `Main` runtime entry point for QuantumOS.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "QuantumOSRuntimeTypes.hpp"

namespace Quantum::Streaming {
  void InitializeStandardStreams() __attribute__((weak));
  void CloseStandardStreams() __attribute__((weak));
}

int Main() __attribute__((weak));

int Main(
  int argumentCount
) __attribute__((weak));

int Main(
  int argumentCount,
  char** arguments
) __attribute__((weak));

int Main(
  int argumentCount,
  char** arguments,
  char** environment
) __attribute__((weak));

using InitFunction = void (*)();
using EntryPointWithNoArguments = int (*)();
using EntryPointWithArgumentCount = int (*)(int);
using EntryPointWithArguments = int (*)(int, char**);
using EntryPoint = int (*)(int, char**, char**);

extern "C" {
  extern InitFunction __init_array_start[];
  extern InitFunction __init_array_end[];
}

namespace {
  /**
   * @brief Calls all global constructors registered in the `.init_array`
   *        section.
   */
  void CallGlobalConstructors() {
    for (
      InitFunction* fn = __init_array_start;
      fn < __init_array_end;
      fn++
    ) {
      (*fn)();
    }
  }
}

/**
 * @brief Parses the initial @p stack and invokes the `Main` entry point.
 * @param stack @ref UInt32 pointer to the stack.
 */
extern "C"
[[noreturn]]
void StartFromStack(UInt32* stack) {
  KernelClient kernelClient;

  int argumentCount = 0;
  char** arguments = nullptr;
  char** environment = nullptr;

  if (stack) {
    argumentCount = static_cast<int>(stack[0]);
    arguments = reinterpret_cast<char**>(&stack[1]);
    environment
      = arguments
      ? (arguments + argumentCount)
      : nullptr;
  }

  EntryPointWithNoArguments mainWithNoArguments
    = static_cast<EntryPointWithNoArguments>(Main);
  EntryPointWithArgumentCount mainWithArgumentCount
    = static_cast<EntryPointWithArgumentCount>(Main);
  EntryPointWithArguments mainWithArguments
    = static_cast<EntryPointWithArguments>(Main);
  EntryPoint main = static_cast<EntryPoint>(Main);
  int exitCode = 0;

  CallGlobalConstructors();

  if (InitializeStandardStreams) {
    InitializeStandardStreams();
  }

  kernelClient.WriteLog(
    LogLevel::Debug,
    "CRT0: stack=%p argumentCount=%u arguments=%p environment=%p "
    "main=%p mainWithArgumentCount=%p mainWithArguments=%p "
    "mainWithNoArguments=%p",
    stack,
    argumentCount,
    arguments,
    environment,
    main,
    mainWithArgumentCount,
    mainWithArguments,
    mainWithNoArguments
  );

  if (main) {
    exitCode = main(
      argumentCount,
      arguments,
      environment
    );
  } else if (mainWithArguments) {
    exitCode = mainWithArguments(
      argumentCount,
      arguments
    );
  } else if (mainWithArgumentCount) {
    exitCode = mainWithArgumentCount(argumentCount);
  } else if (mainWithNoArguments) {
    exitCode = mainWithNoArguments();
  }

  if (CloseStandardStreams) {
    CloseStandardStreams();
  }

  kernelClient.WriteLog(
    LogLevel::Debug,
    "CRT0: exitCode=%d",
    exitCode
  );
  kernelClient.ExitProcess(exitCode);

  for (;;) {}
}

/**
 * @brief C runtime start function.
 *
 * Invokes the `Main` entry point and exits via @ref KernelClient.
 */
extern "C"
[[gnu::section(".text.start")]]
[[gnu::naked]]
void Start() {
  asm volatile(
    "mov %esp, %eax\n"
    "push %eax\n"
    "call StartFromStack\n"
    "add $4, %esp\n"
    "hlt\n"
  );
}
