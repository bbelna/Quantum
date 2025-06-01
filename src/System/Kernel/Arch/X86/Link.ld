/*-------------------------------------------------------------------------------
  Quantum
 -------------------------------------------------------------------------------
  src/System/Kernel/Arch/X86/Link.ld

  A minimal 32-bit GDT-aware linker script. It:
   • Forces ELF32-i386 output
   • Specifies the entry point (EnterProtectedMode)
   • Places .text, .rodata, .data, and .bss at 0x8000 (where the bootloader expects
     the kernel to live).
 -------------------------------------------------------------------------------*/

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(EnterProtectedMode)

SECTIONS
{
    /* Start placing all loadable sections at 0x8000 (the address the bootloader jumps to) */
    . = 0x00008000;

    .text :
    {
        *(.text)           /* all .text from your .o files */
        *(.text*)          /* any other text-subsections (constructors, etc.) */
    }

    .rodata :            /* read-only data (string literals, etc.) */
    {
        *(.rodata)
        *(.rodata*)
    }

    .data :              /* initialized data */
    {
        *(.data)
        *(.data*)
    }

    .bss :               /* uninitialized data (zeroed at startup) */
    {
        __bss_start = .; /* (optional) allow kernel code to know where .bss begins */
        *(.bss)
        *(.bss*)
        __bss_end = .;   /* (optional) and where .bss ends */
    }
}
