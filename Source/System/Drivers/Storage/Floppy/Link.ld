/* Quantum Floppy driver linker script (user-mode) */
ENTRY(main)

SECTIONS {
  . = 0x00400000;

  .entry : {
    LONG(main - 0x00400000)
  }

  .text.start : {
    KEEP(*(.text.start))
  }

  .text : {
    *(.text*)
  }

  .rodata : {
    *(.rodata*)
  }

  .data : {
    *(.data*)
  }

  .bss : {
    *(.bss*)
    *(COMMON)
  }

  /DISCARD/ : {
    *(.comment)
    *(.note*)
    *(.eh_frame*)
    *(.gnu*)
  }
}
