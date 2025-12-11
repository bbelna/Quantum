//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Types/Writer.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Abstract writer interface for writing data.
//------------------------------------------------------------------------------

#pragma once

#include <Types/String.hpp>

namespace Quantum::Kernel::Types {
  using Types::String;

  /**
   * Abstract writer interface for writing data.
   */
  class Writer {
    public:
      /**
       * Writes a message.
       * @param message The message.
       */
      virtual void Write(String message);

      /**
       * Virtual destructor.
       */
      virtual ~Writer() = default;
  };
}
