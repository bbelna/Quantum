/**
 * @file Servers/Run/RunServerTypes.hpp
 * @brief Declares types for @ref @QRunSrv.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Clients.hpp>
#include <Quantum/Core.hpp>
#include <Quantum/Executable.hpp>
#include <Quantum/Servers/Core.hpp>
#include <Quantum/Servers/FileSystem.hpp>
#include <Quantum/Threading.hpp>
#include <Quantum/Types.hpp>

using namespace Quantum::Clients;
using namespace Quantum::Core;
using namespace Quantum::Executable;
using namespace Quantum::Servers::Core;

namespace Quantum::Servers::Run {
  static inline KernelClient Kernel;

  namespace Controllers {
    class LoadController;
    class ProcessController;
  }

  namespace Core {
    namespace ELF {
      enum class ELFSegmentFlags : UInt32;
      enum class ELFSegmentType : UInt32;
    }

    namespace ELF32 {
      struct ELF32Header;
      struct ELF32ProgramHeader;

      class ELF32Loader;

      bool ValidateELF32Header(
        const UInt8* image,
        Size size
      );

      const ELF32ProgramHeader* GetELF32ProgramHeader(
        const UInt8* image,
        UInt32 index
      );
    }

    namespace Process {
      struct ProcessEntry;

      class ProcessTable;
    }
  }
}

using namespace Quantum::Servers::Run;
using namespace Quantum::Servers::Run::ABI;
using namespace Quantum::Servers::Run::Controllers;
using namespace Quantum::Servers::Run::Core::ELF;
using namespace Quantum::Servers::Run::Core::ELF32;
using namespace Quantum::Servers::Run::Core::Process;
