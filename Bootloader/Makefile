# Makefile for IA-32 BIOS boot stages (floppy)
#
# Expects PROJECT_ROOT, BUILD, ARCH, BOOT_MEDIUM from the top-level
# Makefile. Defaults allow standalone invocation from this directory.

PROJECT_ROOT ?= $(abspath ..)

-include $(PROJECT_ROOT)/Makefile.common.mk

ASM     ?= nasm
ASFLAGS ?= -f bin

all: IA32PCFloppy IA32PCHDD Boot

# ---------------------------------------------------------------------------- #
# Stage 1 / Stage 2 (16-bit BIOS, raw binary)
# ---------------------------------------------------------------------------- #

BOOTLOADER_PC_FLOPPY_SRC  := Platform/PC/Bootstrap/Floppy
BOOTLOADER_IA32_FLOPPY_STAGE1_SRCS := \
	$(BOOTLOADER_PC_FLOPPY_SRC)/Stage1.asm \
	$(BOOTLOADER_PC_FLOPPY_SRC)/VGA.inc
BOOTLOADER_IA32_FLOPPY_STAGE2_SRCS := \
	$(BOOTLOADER_PC_FLOPPY_SRC)/Stage2.asm \
	$(BOOTLOADER_PC_FLOPPY_SRC)/VGA.inc

BOOTLOADER_IA32_FLOPPY_ASFLAGS := -I$(BOOTLOADER_PC_FLOPPY_SRC)

IA32PCFloppy: IA32PCFloppyStage1 IA32PCFloppyStage2

IA32PCFloppyStage1: $(BOOTLOADER_IA32_PC_FLOPPY_STAGE1)

IA32PCFloppyStage2: $(BOOTLOADER_IA32_PC_FLOPPY_STAGE2)

$(BOOTLOADER_IA32_PC_FLOPPY_STAGE1): $(BOOTLOADER_IA32_FLOPPY_STAGE1_SRCS)
	@mkdir -p $(dir $@)
	$(call ECHO,ASM $(REL)/$<,Quantum.Bootloader)
	@$(ASM) $(ASFLAGS) $(BOOTLOADER_IA32_FLOPPY_ASFLAGS) $< -o $@

$(BOOTLOADER_IA32_PC_FLOPPY_STAGE2): $(BOOTLOADER_IA32_FLOPPY_STAGE2_SRCS)
	@mkdir -p $(dir $@)
	$(call ECHO,ASM $(REL)/$<,Quantum.Bootloader)
	@$(ASM) $(ASFLAGS) $(BOOTLOADER_IA32_FLOPPY_ASFLAGS) $< -o $@

# ---------------------------------------------------------------------------- #
# Stage 1 / Stage 2 - HDD (16-bit BIOS, raw binary, INT 13h extensions)
# ---------------------------------------------------------------------------- #

BOOTLOADER_PC_HDD_SRC  := Platform/PC/Bootstrap/HDD
BOOTLOADER_IA32_HDD_STAGE1_SRCS := \
	$(BOOTLOADER_PC_HDD_SRC)/Stage1.asm \
	$(BOOTLOADER_PC_HDD_SRC)/VGA.inc
BOOTLOADER_IA32_HDD_STAGE2_SRCS := \
	$(BOOTLOADER_PC_HDD_SRC)/Stage2.asm \
	$(BOOTLOADER_PC_HDD_SRC)/VGA.inc

BOOTLOADER_IA32_HDD_ASFLAGS := -I$(BOOTLOADER_PC_HDD_SRC)

IA32PCHDD: IA32PCHDDStage1 IA32PCHDDStage2

IA32PCHDDStage1: $(BOOTLOADER_IA32_PC_HDD_STAGE1)

IA32PCHDDStage2: $(BOOTLOADER_IA32_PC_HDD_STAGE2)

$(BOOTLOADER_IA32_PC_HDD_STAGE1): $(BOOTLOADER_IA32_HDD_STAGE1_SRCS)
	@mkdir -p $(dir $@)
	$(call ECHO,ASM $(REL)/$<,Quantum.Bootloader)
	@$(ASM) $(ASFLAGS) $(BOOTLOADER_IA32_HDD_ASFLAGS) $< -o $@

$(BOOTLOADER_IA32_PC_HDD_STAGE2): $(BOOTLOADER_IA32_HDD_STAGE2_SRCS)
	@mkdir -p $(dir $@)
	$(call ECHO,ASM $(REL)/$<,Quantum.Bootloader)
	@$(ASM) $(ASFLAGS) $(BOOTLOADER_IA32_HDD_ASFLAGS) $< -o $@

# ---------------------------------------------------------------------------- #
# Boot (32-bit C++ bootloader, loaded as a FAT12 file)
# ---------------------------------------------------------------------------- #

BOOT_SRC          := .
BOOT_X86_SRC      := Arch/x86
BOOT_PLATFORM_SRC := Platform/PC
BOOT_HAL_SRC      := Platform/PC/HAL
BOOT_SHELL_SRC    := Shell
BOOT_BUILD        := $(BUILD)/Boot
BOOT_INCLUDES     := -I$(BOOT_SRC) -I$(BOOT_X86_SRC) -I$(BOOT_PLATFORM_SRC) -I$(BOOT_HAL_SRC) -I$(BOOT_SHELL_SRC) -I$(PROJECT_INCLUDE)

BOOT_CPPS      := $(wildcard $(BOOT_SRC)/*.cpp) $(wildcard $(BOOT_X86_SRC)/*.cpp) $(wildcard $(BOOT_PLATFORM_SRC)/*.cpp) $(wildcard $(BOOT_HAL_SRC)/*.cpp) $(wildcard $(BOOT_SHELL_SRC)/*.cpp)
BOOT_ASMS      := $(wildcard $(BOOT_X86_SRC)/*.asm) $(wildcard $(BOOT_PLATFORM_SRC)/*.asm) $(wildcard $(BOOT_HAL_SRC)/*.asm)
BOOT_CPP_OBJS  := $(patsubst %.cpp,$(BOOT_BUILD)/%.cpp.o,$(BOOT_CPPS))
BOOT_ASM_OBJS  := $(patsubst %.asm,$(BOOT_BUILD)/%.asm.o,$(BOOT_ASMS))
BOOT_OBJS      := $(BOOT_ASM_OBJS) $(BOOT_CPP_OBJS)

Boot: $(BOOTLOADER_BOOT)

$(BOOT_BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Bootloader)
	@$(CC32) $(CFLAGS32) $(BOOT_INCLUDES) -c $< -o $@

$(BOOT_BUILD)/%.asm.o: %.asm
	@mkdir -p $(dir $@)
	$(call ECHO,ASM $(REL)/$<,Quantum.Bootloader)
	@$(ASM) -f elf32 $< -o $@

$(BOOTLOADER_BOOT): $(BOOT_OBJS) $(BOOTLOADER_LINKER)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Bootloader)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-T $(BOOTLOADER_LINKER) \
		$(BOOT_OBJS) -o $@.elf
	$(call ECHO,OBJCOPY $(REL)/$@,Quantum.Bootloader)
	@$(OBJCOPY32) -O binary $@.elf $@

-include $(BOOT_CPP_OBJS:.o=.d)

clean:
	$(call ECHO,RM $(BUILD),Quantum.Bootloader)
	@rm -rf $(BUILD)

.PHONY: all clean IA32PCFloppy IA32PCFloppyStage1 IA32PCFloppyStage2 IA32PCHDD IA32PCHDDStage1 IA32PCHDDStage2 Boot
