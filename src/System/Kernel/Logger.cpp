//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Logger.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Kernel logging and tracing interface.
//------------------------------------------------------------------------------

#include <KernelTypes.hpp>
#include <Logging/Logger.hpp>

namespace Quantum::Kernel::Logging {
  void Logger::Log(Level level, String format, ...) {
    VariableArgumentsList args;
    VARIABLE_ARGUMENTS_START(args, format);

    // TODO: dispatch to configured sinks
    (void)level;
    (void)format;

    VARIABLE_ARGUMENTS_END(args);
  }
}
