# Quantum

Quantum is just another hobbyist operating system.

## v0.01 TODO

**Boot/runtime**
- [ ] Enable higher-half kernel mapping; relocate sections and adjust paging/entry.
- [ ] Clean handoff path: BSS clear, stack setup, boot info validation, sane panic path.
- [ ] Robust memory map parsing with sanity checks; fallback defaults.

**Memory management**
- [x] Physical page allocator with tracking and guard pages.
- [x] Kernel heap with sanity checks; self-test hooks.
- [ ] Virtual memory abstraction: map/unmap helpers, page fault handler stubs/logging.
- [ ] Heap validation improvements (double-free, bounds checks); optional debug fills.

**Interrupts and CPU**
- [x] IDT/ISR coverage with basic handlers and fault reporting.
- [x] PIC remap, IRQ masking/unmasking; timer tick verified.
- [ ] Basic CPU feature probes (cpuid) and safe halting loop.

**Drivers/IO**
- [x] VGA text console (scrolling, cursor) with formatted output.
- [x] PS/2 keyboard input with buffer.
- [ ] Serial output for debug (COM1).

**Logging/Diagnostics**
- [ ] Logger implementation with sinks (console, serial); log levels and prefixes.
- [x] Panic output includes file:line:function; memory state dump on request.
- [x] Self-test hooks for memory (build-flag gated).

**Shell and programs**
- [ ] Minimal kernel shell: prompt, command parsing, help/version, memory stats, reboot/halt.
- [ ] Simple “program” loader stub: define an in-memory format or placeholder to load/jump to a test payload.
- [ ] Basic userland API stubs (write/exit) for future programs.

**Build/config**
- [ ] Clean config flags: `KERNEL_SELF_TEST`, `KERNEL_SELF_TEST_VERBOSE`, logging level, higher-half toggle.
- [ ] README updated with build/run steps, QEMU/bochs invocation, and current feature checklist.

**Hardening/future**
- [ ] Page fault handler that logs and panics with faulting address/flags.
- [ ] Stack guards (guard pages), basic sanity for task switch (future multitasking).
- [ ] Architecture abstraction cleanups to ease adding new backends.
