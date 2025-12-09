OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(KernelEntry)

SECTIONS
{
    /* Physical load address for the kernel */
    . = 0x00010000;
    __phys_start = .;

    /* All executable code, in one contiguous region */
    .text :
    {
        KEEP(*(.text.start))    /* KernelEntry first */
        *(.text)                /* other code */
        *(.text.*)
        __text_end = .;
    }

    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data :
    {
        *(.data)
        *(.data.*)
    }

    .bss :
    {
        __bss_start = .;
        *(COMMON)
        *(.bss)
        *(.bss.*)
        __bss_end = .;
    }

    __phys_end = __bss_end;

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
        *(.gnu*)
    }
}
