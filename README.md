# Quantum
(c) Brandon Belna 2025 - MIT License

## Overview
Just another hobbyist operating system.
Clean, readable, and documented code is at the heart of its philosophy.
It does not try to emulate any existing operating system architectures:
it is built from the ground-up to be something new, sleek, efficient, and
modern.

How far will it go? Until I'm bored I suppose. Long-term ideas and the current
roadmap are outlined below (after Building & Debugging).

## Building & Debugging

### Windows (Ubuntu WSL)
Install the stuff you need in WSL:
```bash
sudo apt update
sudo apt install -y build-essential nasm mtools dosfstools
```

Then update `.\Tools\Build.ps1` with the project path on your Windows machine
and from within WSL.

Now you're all set up. In a normal non-WSL PowerShell, run `.\Build` to build.
If successful, a floppy disk image will be outputted to `Build\Quantum.img`.

To build a version that performs kernel testing, run `.\Build -t`.

Using the debugging script requires QEMU to be installed and your PATH set up
to point to QEMU's directory. 
To build and debug, run `.\Build -r`. To just debug, run `.\Debug`.
You can do `.\Build -r -t` to build and debug a version that runs tests.


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
- [x] Ensure consistent use of kernel types (e.g., `const char*` should be `CString` or `String`).
- [x] Refactor all private namespace variables to use _ prefix.
- [x] Replace anonymous namespace blocks with class-private members.
- [x] Ensure all functions and properties are documented.
- [x] Wrap IDT.cpp in a class.

**Boot/runtime**
- [x] Enable higher-half kernel mapping; relocate sections and adjust paging/entry.
- [x] Clean handoff path: BSS clear, stack setup, boot info validation, sane panic path.
- [x] INIT.BND mapping and coordinator launch moved into InitBundle helper.
- [ ] Robust memory map parsing with sanity checks; fallback defaults.

**Memory management**
- [x] Physical page allocator with tracking and guard pages.
- [x] Kernel heap with sanity checks; self-test hooks.
- [x] Virtual memory abstraction: map/unmap helpers, page fault handler stubs/logging.
- [x] Per-task address spaces and TSS kernel stack updates for user mode.
- [x] Kernel heap moved to higher-half with a larger reserved region.
- [ ] Heap validation improvements (double-free, bounds checks); optional debug fills.

**Interrupts and CPU**
- [x] IDT/ISR coverage with basic handlers and fault reporting.
- [x] PIC remap, IRQ masking/unmasking; timer tick verified.
- [x] Basic CPU feature probes (cpuid) and safe halting loop.

**Drivers/IO**
- [x] VGA text console (scrolling, cursor) with formatted output.
- [x] PS/2 keyboard input with buffer.
- [x] IA32 port I/O refactored into a class with CPP implementation.
- [ ] Serial output for debug (COM1).

**Logging/Diagnostics**
- [x] Logger implementation with sinks (console, serial); log levels and prefixes.
- [x] Panic output includes file:line:function; memory state dump on request.
- [x] Self-test hooks for memory (build-flag gated).

**Build/config**
- [ ] Clean config flags: `KERNEL_SELF_TEST`, `KERNEL_SELF_TEST_VERBOSE`, logging level, higher-half toggle.
- [ ] README updated with build/run steps, QEMU/bochs invocation, and current feature checklist.

**Hardening/future**
- [ ] Page fault handler that logs and panics with faulting address/flags.
- [ ] Stack guards (guard pages), basic sanity for task switch (future multitasking).
- [ ] Architecture abstraction cleanups to ease adding new backends.
