ENTRY(Start)

PHDRS {
  text PT_LOAD FLAGS(5); /* R + X */
  data PT_LOAD FLAGS(6); /* R + W */
}

SECTIONS {
  . = 0x40000000;

  .entry : {
    LONG(Start - 0x40000000)
    LONG(__image_end - 0x40000000)
  } :text

  .text.start : {
    KEEP(*(.text.start))
  } :text

  .text : {
    *(.text*)
  } :text

  .rodata : {
    *(.rodata*)
  } :text

  .init_array : {
    __init_array_start = .;
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    __init_array_end = .;
  } :text

  .data : {
    *(.data*)
  } :data

  .bss : {
    *(.bss*)
    *(COMMON)
  } :data

  __image_end = .;

  /DISCARD/ : {
    *(.comment)
    *(.note*)
    *(.eh_frame*)
    *(.gnu*)
  }
}
