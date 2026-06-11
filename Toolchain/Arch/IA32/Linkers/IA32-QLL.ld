/* Linker script for Quantum Loadable Libraries (QLL).
 *
 * Produces a position-independent ELF32 shared object (ET_DYN).
 * Base address is 0; the Run server relocates at load time.
 */

PHDRS {
  text PT_LOAD FLAGS(5); /* R + X */
  data PT_LOAD FLAGS(6); /* R + W */
  dynamic PT_DYNAMIC FLAGS(6); /* R + W */
}

SECTIONS {
  . = 0;

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

  .dynsym : {
    *(.dynsym)
  } :text

  .dynstr : {
    *(.dynstr)
  } :text

  .hash : {
    *(.hash)
  } :text

  .gnu.hash : {
    *(.gnu.hash)
  } :text

  .rel.dyn : {
    *(.rel.dyn)
    *(.rel.text*)
    *(.rel.rodata*)
    *(.rel.data*)
    *(.rel.got*)
    *(.rel.plt*)
  } :text

  . = ALIGN(4096);

  .dynamic : {
    *(.dynamic)
  } :data :dynamic

  .got : {
    *(.got*)
  } :data

  .got.plt : {
    *(.got.plt*)
  } :data

  .plt : {
    *(.plt*)
  } :data

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
    *(.gnu.version*)
  }
}
