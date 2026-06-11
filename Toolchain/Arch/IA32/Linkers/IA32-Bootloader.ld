OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(BootEntry)

BOOT_LOAD_PHYS = 0x5000;

SECTIONS
{
  . = BOOT_LOAD_PHYS;

  .text : {
    KEEP(*(.text.start))
    *(.text)
    *(.text.*)
  }

  .rodata : {
    *(.rodata)
    *(.rodata.*)
  }

  .data : {
    *(.data)
    *(.data.*)
  }

  .bss : {
    __bss_start = .;
    *(COMMON)
    *(.bss)
    *(.bss.*)
    __bss_end = .;
  }

  /DISCARD/ : {
    *(.comment)
    *(.note*)
    *(.eh_frame*)
    *(.gnu*)
  }
}
