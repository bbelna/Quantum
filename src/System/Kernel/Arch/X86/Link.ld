/* OUTPUT_FORMAT("binary") ensures ld emits a raw flat binary */
OUTPUT_FORMAT("binary")

/* The kernel’s true entry point is EnterProtectedMode (from KernelEntry.cpp) */
ENTRY(EnterProtectedMode)

SECTIONS {
  /* 
   * Place all .text, .rodata, .data, .bss at virtual address 0x8000 (i.e. physical 0x0000:0x8000).
   * That means when the bootloader does `jmp 0x0000:0x8000`, it will land on EnterProtectedMode.
   */
  . = 0x8000;

  .text   : { *(.text)   }
  .rodata : { *(.rodata) }
  .data   : { *(.data)   }
  .bss    : { *(COMMON) *(.bss) }
}
