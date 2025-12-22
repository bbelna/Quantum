# Quantum Roadmap

This document details the current roadmap for Quantum. As time goes on, the
roadmap will probably evolve and end up formalized in a system like Github/Jira,
and then we won't really use this anymore. But it's useful for now. 

## v0.01

The aim for v0.01 is an old school floppy disk DOS-esque operating system for
IA32/x86. 

### Bootloader

#### IA32 Floppy Disk
- [x] Locates the kernel by searching the filesystem and loads it.
- [x] Enable higher-half kernel mapping; relocate sections and adjust paging/entry.
- [x] Clean handoff path: BSS clear, stack setup, boot info validation, sane panic path.
- [x] Locates the init bundle by searching the filesystem and loads it.
- [ ] Robust memory map parsing with sanity checks; fallback defaults.

### Kernel

**Note:** All tasks here are accomplished either in the architecture-agnostic
layer or the IA32 layer.

### Architecture
- [x] Clean separation for various architecture implementations.
- [x] Locates and spawns Coordinator after initialization.

#### Memory Management
- [x] Physical page allocator with tracking and guard pages.
- [x] Kernel heap with sanity checks; self-test hooks.
- [x] Virtual memory abstraction: map/unmap helpers, page fault handler stubs/logging.
- [x] Per-task address spaces and TSS kernel stack updates for user mode.
- [x] Kernel heap moved to higher-half with a larger reserved region.
- [ ] Heap validation improvements (double-free, bounds checks); optional debug fills.

#### Interrupts and CPU
- [x] IDT/ISR coverage with basic handlers and fault reporting.
- [x] PIC remap, IRQ masking/unmasking; timer tick verified.
- [x] Basic CPU feature probes (cpuid) and safe halting loop.

#### Devices/IO
- [x] VGA text console (scrolling, cursor) with formatted output.
- [x] PS/2 keyboard input with buffer.
- [x] IA32 port I/O refactored into a class with CPP implementation.
- [ ] Serial output for debug (COM1).
- [x] IRQ floppy disk handler.
- [ ] Device manager with block device support.

#### Tasks
- [x] Basic multitasking.
- [x] Preemptive multitasking.

#### Logging/Diagnostics
- [x] Logger implementation with sinks (console, serial); log levels and prefixes.
- [x] Panic output includes file:line:function; memory state dump on request.
- [x] Self-test hooks for memory (build-flag gated).


#### Testing
- [x] Kernel testing harness: unit tests that are runnable with a build flag.
- [ ] Complete memory test suite.
- [ ] Complete task test suite.

### Drivers
- [ ] Floppy disk driver.

### Services
- [ ] FAT12 file system service.

### Coordinator
- [x] Locates and parses the init bundle.
- [x] Spawns floppy disk driver based on device detection.

### Build and Configuration
- [ ] Clean config flags: `KERNEL_SELF_TEST`, `KERNEL_SELF_TEST_VERBOSE`, logging level, higher-half toggle.
- [ ] README updated with build/run steps, QEMU/bochs invocation, and current feature checklist.
