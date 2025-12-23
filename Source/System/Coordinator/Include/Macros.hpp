/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Coordinator/Include/Macros.hpp
 * Coordinator macros.
 */

#pragma once

#include "Testing.hpp"

#define TEST_ASSERT(cond, msg) \
  ::Quantum::System::Coordinator::Testing::Assert(\
    (cond), (msg), __FILE__, __LINE__\
  )
