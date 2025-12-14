# Quantum
(c) Brandon Belna 2025 - MIT License

## Overview
Just another hobbyist operating system.
Clean, readable, and documented code is at the heart of its philosophy.
It does not try to emulate any existing operating system architectures:
it is built from the ground-up to be something new, sleek, efficient, and modern.

How far will it go? Until I'm bored I suppose. Long-term ideas and the current roadmap are outlined below (after Building & Debugging).

## Building & Debugging

### Windows (Ubuntu WSL)
Install the stuff you need in WSL:
```bash
sudo apt update
sudo apt install -y build-essential nasm mtools dosfstools
```
You can then, in a normal non-WSL PowerShell, run `.\Build` to build. If successful, a floppy disk image will be outputted to `Build\Quantum.img`.

To build and debug, run `.\Build -r`. To just debug, run `.\Debug`.


## Planned Hardware Support
  * **Architectures**: IA32, AMD64.
  * **Storage**: CD/DVD, HDD, USB.
  * **Graphics**: As long as we have colored graphics support generally.
  * **Networking**: At least some proof of concepts.
    * Port of FreeBSD stack (a la Haiku) for fuller support?

## Vision 
* Efficient use of symmetric multiprocessing.
* Native API that focus on clean, optimized, readable code.
* Standard library support (e.g., libc) to faciliate porting
  existing applications.
* Cool, new, and fun GUI.
  * We will have a text-mode shell along the way; eventually will be
    supplanted by a GUI.

## Roadmap
### v0.01

**Code cleanliness**
- [ ] Ensure consistent use of kernel types (e.g., `const char*` should be `CString` or `String`).
- [ ] Refactor all private namespace variables to use _ prefix.
- [ ] Ensure all functions and properties are documented.
- [ ] Wrap IDT.cpp in a class.

**Boot/runtime**
- [x] Enable higher-half kernel mapping; relocate sections and adjust paging/entry.
- [ ] Clean handoff path: BSS clear, stack setup, boot info validation, sane panic path.
- [ ] Robust memory map parsing with sanity checks; fallback defaults.

**Memory management**
- [x] Physical page allocator with tracking and guard pages.
- [x] Kernel heap with sanity checks; self-test hooks.
- [x] Virtual memory abstraction: map/unmap helpers, page fault handler stubs/logging.
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
- [x] Logger implementation with sinks (console, serial); log levels and prefixes.
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
