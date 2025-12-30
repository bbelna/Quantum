/**
 * @file System/Kernel/Thread.cpp
 * @brief Architecture-agnostic thread management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright Ac 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Thread.hpp"

namespace Quantum::System::Kernel {
  void Thread::Initialize() {
    Arch::Thread::Initialize();
  }

  Thread::ControlBlock* Thread::Create(
    TaskControlBlock* task,
    void (*entryPoint)(),
    UInt32 stackSize
  ) {
    return reinterpret_cast<Thread::ControlBlock*>(
      Arch::Thread::Create(task, entryPoint, stackSize)
    );
  }

  Thread::ControlBlock* Thread::CreateUser(
    TaskControlBlock* task,
    UInt32 entryPoint,
    UInt32 userStackTop,
    UInt32 stackSize
  ) {
    return reinterpret_cast<Thread::ControlBlock*>(
      Arch::Thread::CreateUser(task, entryPoint, userStackTop, stackSize)
    );
  }

  void Thread::Exit() {
    Arch::Thread::Exit();
  }

  void Thread::Yield() {
    Arch::Thread::Yield();
  }

  Thread::ControlBlock* Thread::GetCurrent() {
    return reinterpret_cast<Thread::ControlBlock*>(Arch::Thread::GetCurrent());
  }

  UInt32 Thread::GetCurrentId() {
    Thread::ControlBlock* tcb = GetCurrent();

    return tcb ? tcb->id : 0;
  }

  void Thread::EnablePreemption() {
    Arch::Thread::EnablePreemption();
  }

  void Thread::DisablePreemption() {
    Arch::Thread::DisablePreemption();
  }

  Interrupts::Context* Thread::Tick(Interrupts::Context& context) {
    return Arch::Thread::Tick(context);
  }

  void Thread::Wake(Thread::ControlBlock* thread) {
    Arch::Thread::Wake(thread);
  }
}
