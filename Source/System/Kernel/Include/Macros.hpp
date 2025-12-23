/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Macros.hpp
 * Kernel macros.
 */

#pragma once

#include "Handlers/PanicHandler.hpp"
#include "Testing.hpp"

#define PANIC(msg) \
  ::Quantum::System::Kernel::Handlers::PanicHandler::Handle(\
    (msg), __FILE__, __LINE__, __FUNCTION__\
  )

#define TEST_ASSERT(cond, msg) \
  ::Quantum::System::Kernel::Testing::Assert((cond), (msg), __FILE__, __LINE__)
