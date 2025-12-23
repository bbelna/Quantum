/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Task.cpp
 * Architecture-agnostic task management.
 */

#include "Interrupts.hpp"
#include "Logger.hpp"
#include "Task.hpp"
#include "Types.hpp"

#if defined(QUANTUM_ARCH_IA32)
#include "Arch/IA32/Task.hpp"
#endif

namespace Quantum::System::Kernel {
  using LogLevel = Logger::Level;

  #if defined(QUANTUM_ARCH_IA32)
  using ArchTask = Arch::IA32::Task;
  #endif

  void Task::Initialize() {
    ArchTask::Initialize();
    EnablePreemption();
  }

  Task::ControlBlock* Task::Create(void (*entryPoint)(), UInt32 stackSize) {
    return reinterpret_cast<Task::ControlBlock*>(
      ArchTask::Create(entryPoint, stackSize)
    );
  }

  Task::ControlBlock* Task::CreateUser(
    UInt32 entryPoint,
    UInt32 userStackTop,
    UInt32 pageDirectoryPhysical
  ) {
    #if defined(QUANTUM_ARCH_IA32)
    return reinterpret_cast<Task::ControlBlock*>(
      ArchTask::CreateUser(entryPoint, userStackTop, pageDirectoryPhysical)
    );
    #else
    (void)entryPoint;
    (void)userStackTop;
    (void)pageDirectoryPhysical;

    return nullptr;
    #endif
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

  void Task::SetCurrentAddressSpace(UInt32 pageDirectoryPhysical) {
    #if defined(QUANTUM_ARCH_IA32)
    ArchTask::SetCurrentAddressSpace(pageDirectoryPhysical);
    #else
    (void)pageDirectoryPhysical;
    #endif
  }

  UInt32 Task::GetCurrentAddressSpace() {
    #if defined(QUANTUM_ARCH_IA32)
    return ArchTask::GetCurrentAddressSpace();
    #else
    return 0;
    #endif
  }

  void Task::SetCoordinatorId(UInt32 taskId) {
    _coordinatorTaskId = taskId;
  }

  bool Task::IsCurrentTaskCoordinator() {
    UInt32 current = GetCurrentId();

    return current != 0 && current == _coordinatorTaskId;
  }

  bool Task::GrantIOAccess(UInt32 taskId) {
    #if defined(QUANTUM_ARCH_IA32)
    auto* tcb = ArchTask::Find(taskId);

    if (!tcb) {
      return false;
    }

    tcb->caps |= ArchTask::CapabilityIo;

    return true;
    #else
    (void)taskId;

    return false;
    #endif
  }

  bool Task::HasIOAccess() {
    #if defined(QUANTUM_ARCH_IA32)
    auto* tcb = ArchTask::GetCurrent();

    return tcb && (tcb->caps & ArchTask::CapabilityIo) != 0;
    #else
    return false;
    #endif
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
