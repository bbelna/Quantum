## Quantum IA32 Floppy Bootloader
© 2025-2026 The Quantum OS Project - GPL 2.0 License

## Overview
This folder contains the implementation for Quantum's IA32 floppy bootloader.

## Specification
`Stage1.asm` is the first stage bootloader and is written into sector 0.
It determines the boot drive, read the next four sectors of the boot drive
(where stage 2 lives) into `0x0600`, and jumps to that address.

`Stage2.asm` is the second stage bootloader and is written into sectors 1-4. It
searches the FAT file system for the kernel (kernel.qx) and the init bundle 
(init.bnd), and loads them into `0x00010000` and `0x00070000`, respectively.
It then stores the boot memory map and information regarding the init bundle for
use by the kernel, enters protected mode, and then jumps to the kernel.
