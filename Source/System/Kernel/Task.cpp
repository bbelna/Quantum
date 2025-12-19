/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Task.cpp
 * Architecture-agnostic task management.
 */

#include <Interrupts.hpp>
#include <Logger.hpp>
#include <Task.hpp>
#include <Types.hpp>

#if defined(QUANTUM_ARCH_IA32)
#include <Arch/IA32/Task.hpp>
#endif

namespace Quantum::System::Kernel {
  #if defined(QUANTUM_ARCH_IA32)
  using ArchTask = Arch::IA32::Task;
  #endif

  using LogLevel = Logger::Level;

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
    auto* tcb = GetCurrent();

    return tcb ? tcb->id : 0;
  }

  void Task::EnablePreemption() {
    ArchTask::EnablePreemption();
  }

  void Task::DisablePreemption() {
    ArchTask::DisablePreemption();
  }

  Interrupts::Context* Task::Tick(Interrupts::Context& context) {
    return ArchTask::Tick(context);
  }
}
