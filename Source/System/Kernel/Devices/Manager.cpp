/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Devices/Manager.cpp
 * Kernel device manager.
 */

#include <Devices/Block.hpp>
#include <Devices/Manager.hpp>

namespace Quantum::System::Kernel::Devices {
  void Manager::Initialize() {
    Block::Initialize();
  }
}
