/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Task.cpp
 * Architecture-agnostic task management.
 */

#include <Logger.hpp>
#include <Task.hpp>
#include <Types/Primitives.hpp>
#include <Types/Logging/LogLevel.hpp>

#if defined(QUANTUM_ARCH_IA32)
#include <Arch/IA32/Task.hpp>

using ArchTask = Quantum::System::Kernel::Arch::IA32::Task;
using ArchTCB = Quantum::System::Kernel::Arch::IA32::Task::ControlBlock;
#endif

namespace Quantum::System::Kernel {
  using LogLevel = Types::Logging::LogLevel;

  void Task::Initialize() {
    ArchTask::Initialize();
  }

  Task::ControlBlock* Task::Create(void (*entryPoint)(), UInt32 stackSize) {
    return reinterpret_cast<Task::ControlBlock*>(
      ArchTask::Create(entryPoint, stackSize)
    );
  }

  void Task::Exit() {
    ArchTask::Exit();
  }

  void Task::Yield() {
    ArchTask::Yield();
  }

  Task::ControlBlock* Task::GetCurrent() {
    return reinterpret_cast<Task::ControlBlock*>(ArchTask::GetCurrent());
  }

  UInt32 Task::GetCurrentId() {
    auto* tcb = ArchTask::GetCurrent();

    return tcb ? tcb->Id : 0;
  }

  void Task::EnablePreemption() {
    ArchTask::EnablePreemption();
  }

  void Task::DisablePreemption() {
    ArchTask::DisablePreemption();
  }

  InterruptContext* Task::Tick(InterruptContext& context) {
    return ArchTask::Tick(context);
  }
}
