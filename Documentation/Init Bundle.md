# Init Bundle
This document details the specification and tooling for the init bundle.

## Specification
- Purpose: single init bundle loaded by the bootloader and handed to the kernel to start user-mode services (Coordinator, drivers, FS). Only the bootloader and kernel need to know its location; all payload discovery happens via the manifest inside the bundle.
- Boot handoff: bootloader loads `INIT.BND` after the kernel (page-aligned, below 16 MB for floppy DMA friendliness), passes `{phys_base, size}` in BootInfo, and marks the range reserved. Kernel maps it read-only and passes virtual base/size to the first user task.
- Format (little-endian):
  - Header: magic `INITBND\0`, version (u16), entryCount (u16), tableOffset (u32), reserved[8].
  - Entry table (entryCount entries): name[32] (ASCII, NUL-terminated), type (u8: 0=unknown,1=init/coordinator,2=driver,3=service), flags (u8: bit0=required), reserved[2], offset (u32), size (u32), checksum (u32, optional 0=unused).
  - Payloads: concatenated binaries, each page-aligned (4 KB).
- Manifest: one entry named `manifest.txt` (or similar) inside the bundle listing load order and roles; Coordinator reads it first.
- Build: a bundler tool will gather built user-mode binaries from `System/Drivers/*` and `System/Services/*`, write the header/table/payload, and emit `Build/INIT.BND`.

### Tooling
- Scripts: `Tools/Bundle.py` (WSL/Makefile)
- Manifest format (JSON array): `[{"name":"coordinator","type":"init","required":true,"path":"Build/Coordinator.bin"}, {"name":"floppy","type":"driver","required":true,"path":"Build/Floppy.bin"}]`
- Names are ASCII ≤31 bytes; payloads are page-aligned (4 KB) in the bundle. Checksums are 0 for now.
- Build integration:
  - `Tools/Build.ps1` runs the PowerShell bundler if `InitManifest.json` exists and copies `Build/INIT.BND` into the floppy image.
  - The Makefile (WSL) runs `Tools/Bundle.py` if `InitManifest.json` exists and copies `Build/INIT.BND` into the floppy image.
  - A sample manifest lives at `InitManifest.sample.json`.
