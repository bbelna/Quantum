OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(Entry)

/* Higher-half virtual base for the kernel image */
KERNEL_VIRT_BASE = 0xC0000000;
IDENTITY_WINDOW_SIZE = 16M;
KERNEL_LOAD_PHYS = 0x00010000;

SECTIONS
{
  /* Physical load address for the kernel image */
  . = KERNEL_LOAD_PHYS;
  __phys_start = .;

  /* Low bootstrap code stays identity mapped */
  .text.start :
  {
    KEEP(*(.text.start.entry))  /* force Entry first */
    KEEP(*(.text.start))
    *(.text.start.data)
  }

  /* Start of the higher-half mapping (VMA) and where that region is loaded (LMA). */
  __hh_virt_start = KERNEL_VIRT_BASE + SIZEOF(.text.start);
  __hh_phys_start = KERNEL_LOAD_PHYS + SIZEOF(.text.start);

  /* Map the rest of the kernel at higher-half VA, but loaded at the same physical LMA */
  .text KERNEL_VIRT_BASE + SIZEOF(.text.start) : AT(KERNEL_LOAD_PHYS + SIZEOF(.text.start))
  {
    *(.text)                /* other code */
    *(.text.*)
    __text_end = .;
  }

  .rodata : AT(ADDR(.rodata) - KERNEL_VIRT_BASE + KERNEL_LOAD_PHYS)
  {
    *(.rodata)
    *(.rodata.*)
  }

  .init_array : AT(ADDR(.init_array) - KERNEL_VIRT_BASE + KERNEL_LOAD_PHYS)
  {
    __init_array_start = .;
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    __init_array_end = .;
  }

  .fini_array : AT(ADDR(.fini_array) - KERNEL_VIRT_BASE + KERNEL_LOAD_PHYS)
  {
    __fini_array_start = .;
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array))
    __fini_array_end = .;
  }

  .data : AT(ADDR(.data) - KERNEL_VIRT_BASE + KERNEL_LOAD_PHYS)
  {
    *(.data)
    *(.data.*)
  }

  .bss : AT(ADDR(.bss) - KERNEL_VIRT_BASE + KERNEL_LOAD_PHYS)
  {
    __bss_start = .;
    *(COMMON)
    *(.bss)
    *(.bss.*)
    __bss_end = .;
  }

  __phys_end = __bss_end - KERNEL_VIRT_BASE + KERNEL_LOAD_PHYS;
  __virt_start = KERNEL_VIRT_BASE;
  __virt_end = .;
  __phys_bss_start = __bss_start - KERNEL_VIRT_BASE + KERNEL_LOAD_PHYS;
  __phys_bss_end = __bss_end - KERNEL_VIRT_BASE + KERNEL_LOAD_PHYS;

  /DISCARD/ :
  {
    *(.comment)
    *(.note*)
    *(.eh_frame*)
    *(.gnu*)
  }
}
