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

Then update `.\Tools\Build.ps1` with the project path on your Windows machine and from within WSL.

Now you're all set up. In a normal non-WSL PowerShell, run `.\Build` to build. If successful, a floppy disk image will be outputted to `Build\Quantum.img`.

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
- [ ] Ensure consistent use of kernel types (e.g., `const char*` should be `CString` or `String`).
- [x] Refactor all private namespace variables to use _ prefix.
- [x] Ensure all functions and properties are documented.
- [x] Wrap IDT.cpp in a class.

**Boot/runtime**
- [x] Enable higher-half kernel mapping; relocate sections and adjust paging/entry.
- [x] Clean handoff path: BSS clear, stack setup, boot info validation, sane panic path.
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

## INIT.BND boot bundle (planned)
- Purpose: single init bundle loaded by the bootloader and handed to the kernel to start user-mode services (Coordinator, drivers, FS). Only the bootloader and kernel need to know its location; all payload discovery happens via the manifest inside the bundle.
- Boot handoff: bootloader loads `INIT.BND` after the kernel (page-aligned, below 16 MB for floppy DMA friendliness), passes `{phys_base, size}` in BootInfo, and marks the range reserved. Kernel maps it read-only and passes virtual base/size to the first user task.
- Format (little-endian):
  - Header: magic `INITBND\0`, version (u16), entryCount (u16), tableOffset (u32), reserved[8].
  - Entry table (entryCount entries): name[32] (ASCII, NUL-terminated), type (u8: 0=unknown,1=init/coordinator,2=driver,3=service), flags (u8: bit0=required), reserved[2], offset (u32), size (u32), checksum (u32, optional 0=unused).
  - Payloads: concatenated binaries, each page-aligned (4 KB).
- Manifest: one entry named `manifest.txt` (or similar) inside the bundle listing load order and roles; Coordinator reads it first.
- Build: a bundler tool will gather built user-mode binaries from `System/Drivers/*` and `System/Services/*`, write the header/table/payload, and emit `Build/INIT.BND`.

### INIT.BND bundling (tooling)
- Scripts: PowerShell `Tools/Bundle.ps1` (Windows host) or Python `Tools/Bundle.py` (WSL/Makefile)
- Manifest format (JSON array): `[{"name":"coordinator","type":"init","required":true,"path":"Build/Coordinator.bin"}, {"name":"floppy","type":"driver","required":true,"path":"Build/Floppy.bin"}]`
- Names are ASCII ≤31 bytes; payloads are page-aligned (4 KB) in the bundle. Checksums are 0 for now.
- Build integration:
  - `Tools/Build.ps1` runs the PowerShell bundler if `InitManifest.json` exists and copies `Build/INIT.BND` into the floppy image.
  - The Makefile (WSL) runs `Tools/Bundle.py` if `InitManifest.json` exists and copies `Build/INIT.BND` into the floppy image.
  - A sample manifest lives at `InitManifest.sample.json`.
