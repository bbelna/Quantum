/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Task.cpp
 * Architecture-agnostic task management.
 */

#include <Logger.hpp>
#include <Task.hpp>
#include <Types/Logging/Level.hpp>
#include <Types/Primitives.hpp>

#if defined(QUANTUM_ARCH_IA32)
  #include <Arch/IA32/Task.hpp>

  using ArchTask = Quantum::System::Kernel::Arch::IA32::Task;
  using ArchTCB = Quantum::System::Kernel::Arch::IA32::TaskControlBlock;
#else
  #error "No architecture selected for task management"
#endif

namespace Quantum::System::Kernel {
  using LogLevel = Types::Logging::Level;

  void Task::Initialize() {
    ArchTask::Initialize();
  }

  TaskControlBlock* Task::Create(void (*entryPoint)(), UInt32 stackSize) {
    return reinterpret_cast<TaskControlBlock*>(
      ArchTask::Create(entryPoint, stackSize)
    );
  }

  void Task::Exit() {
    ArchTask::Exit();
  }

  void Task::Yield() {
    ArchTask::Yield();
  }

  TaskControlBlock* Task::GetCurrent() {
    return reinterpret_cast<TaskControlBlock*>(ArchTask::GetCurrent());
  }

  void Task::EnablePreemption() {
    ArchTask::EnablePreemption();
  }

  void Task::DisablePreemption() {
    ArchTask::DisablePreemption();
  }

  void Task::Tick() {
    ArchTask::Tick();
  }
}
