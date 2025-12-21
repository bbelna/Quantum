/* Quantum Coordinator linker script (user-mode) */
ENTRY(Start)

SECTIONS {
  . = 0x00400000;

  .entry : {
    LONG(Start - 0x00400000)
    LONG(__image_end - 0x00400000)
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

  __image_end = .;

  /DISCARD/ : {
    *(.comment)
    *(.note*)
    *(.eh_frame*)
    *(.gnu*)
  }
}
