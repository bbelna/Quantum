# Makefile (top‐level) for Quantum
# Builds a two‐stage FAT12 floppy image (1.44 MB) containing:
#   • Sector 0: Boot.asm (Stage 1)
#   • Sector 1: Floppy.asm (Stage 2 FAT12 loader)
#   • Root directory file: QKRNL.QX (the kernel)
#
# All intermediates and outputs go under build/. Source remains untouched.
#
# Usage:
#   make        # → build/Quantum.img (FAT12 floppy)
#   make clean  # → remove build/

#───────────────────────────────────────────────────────────────────────────────
# Directory Variables
#───────────────────────────────────────────────────────────────────────────────

ROOT_DIR       := $(CURDIR)/src
BUILD_DIR      := $(CURDIR)/build

# Bootloader sources
BL_DIR         := $(ROOT_DIR)/System/Boot
BL_BOOT_SRC    := $(BL_DIR)/Boot.asm
BL_FLOPPY_SRC  := $(BL_DIR)/Floppy/Floppy.asm

# Common includes for Boot and Stage 2
BL_COMMON_DIR  := $(BL_DIR)/Common

# Kernel sources
KERNEL_SRC_DIR := $(ROOT_DIR)/System/Kernel
KERNEL_ARCH32  := $(KERNEL_SRC_DIR)/Arch/X86
KERNEL_COMMON  := $(KERNEL_SRC_DIR)
KERNEL_INCLUDE := $(KERNEL_SRC_DIR)/Include

#───────────────────────────────────────────────────────────────────────────────
# Toolchain Definitions
#───────────────────────────────────────────────────────────────────────────────

ASM       := nasm
ASFLAGS   := -f bin -I$(BL_COMMON_DIR)

# x86 (32-bit) compiler/linker settings
CC32      := x86_64-linux-gnu-gcc
CFLAGS32 := -fno-pic -fno-pie -m32 -ffreestanding -O2 -Wall -fno-exceptions \
            -fno-rtti -nostdinc -nostdinc++
LD32      := x86_64-linux-gnu-ld
LDFLAGS32 := --no-pie -m elf_i386

OBJCOPY32 := x86_64-linux-gnu-objcopy

# Utilities for FAT12 image creation (mtools)
MKFS      := mkfs.vfat
MCOPY     := mcopy

#───────────────────────────────────────────────────────────────────────────────
# Stage 1: Assemble Boot.asm → build/Boot/Boot.bin (512 bytes)
#───────────────────────────────────────────────────────────────────────────────

BOOT_STAGE1_BIN := $(BUILD_DIR)/Boot/Boot.bin

$(BOOT_STAGE1_BIN): \
		$(BL_BOOT_SRC) \
		$(BL_COMMON_DIR)/Constants.inc \
		$(BL_COMMON_DIR)/Print.inc
	@mkdir -p $(dir $@)
	$(ASM) $(ASFLAGS) $(BL_BOOT_SRC) -o $(BOOT_STAGE1_BIN)
	@echo "[OK] Assembled Stage 1 (Boot.bin) → $@"

#───────────────────────────────────────────────────────────────────────────────
# Stage 2: Assemble Floppy.asm → build/Boot/Floppy.bin (512 bytes)
#───────────────────────────────────────────────────────────────────────────────

BOOT_STAGE2_BIN := $(BUILD_DIR)/Boot/Floppy.bin

$(BOOT_STAGE2_BIN): \
		$(BL_FLOPPY_SRC) \
		$(BL_COMMON_DIR)/Constants.inc \
		$(BL_COMMON_DIR)/Print.inc
	@mkdir -p $(dir $@)
	$(ASM) $(ASFLAGS) $(BL_FLOPPY_SRC) -o $(BOOT_STAGE2_BIN)
	@echo "[OK] Assembled Stage 2 (Floppy.bin) → $@"

#───────────────────────────────────────────────────────────────────────────────
# Kernel (x86_32) Build
#───────────────────────────────────────────────────────────────────────────────

KER_COMMON_SRCS := \
	$(KERNEL_COMMON)/Kernel.cpp \
	$(KERNEL_COMMON)/Drivers/Console.cpp

KER32_OBJ_DIR := $(BUILD_DIR)/Kernel/X86
GDT_SRC := $(KERNEL_ARCH32)/GDT.asm
GDT_OBJ := $(KER32_OBJ_DIR)/GDT.o
KER32_OBJS    := \
	$(patsubst $(KERNEL_COMMON)/%.cpp,$(KER32_OBJ_DIR)/%.o,$(KER_COMMON_SRCS)) \
	$(GDT_OBJ) \
	$(KER32_OBJ_DIR)/KernelEntry.o

KER32_ELF     := $(KER32_OBJ_DIR)/qkrnl.elf
KER32_BIN     := $(KER32_OBJ_DIR)/qkrnl.qx

# Compile each .cpp → 32-bit .o. 
# We keep the corresponding .hpp as a dependency so that changing it rebuilds the .o,
# but we do not compile the .hpp on its own.
$(KER32_OBJ_DIR)/%.o: $(KERNEL_COMMON)/%.cpp $(KERNEL_INCLUDE)/%.hpp
	@mkdir -p $(dir $@)
	$(CC32) $(CFLAGS32) -I$(KERNEL_INCLUDE) -c $< -o $@

# Compile KernelEntry.cpp → object; depend on its header so that edits trigger rebuild
$(KER32_OBJ_DIR)/KernelEntry.o: $(KERNEL_ARCH32)/KernelEntry.cpp
	@mkdir -p $(dir $@)
	$(CC32) $(CFLAGS32) -I$(KERNEL_INCLUDE) -c $< -o $@

# Assemble GDT.asm into GDT.o
$(GDT_OBJ): $(GDT_SRC)
	@mkdir -p $(dir $@)
	$(ASM) -f elf32 $< -o $@

# Link x86_32 objects → ELF (use gcc driver instead of ld)
$(KER32_ELF): $(KER32_OBJS) $(KERNEL_ARCH32)/Link.ld
	@mkdir -p $(dir $@)
	$(CC32) $(CFLAGS32) -m32 -nostdlib -static -Wl,--no-pie \
	        -T $(KERNEL_ARCH32)/Link.ld \
	        $(KER32_OBJS) -o $@
	@echo "[OK] Linked qkrnl.elf → $@"


# Objcopy ELF → flat binary qkrnl.qx
$(KER32_BIN): $(KER32_ELF)
	$(OBJCOPY32) -O binary $< $@
	@echo "[OK] Created qkrnl.qx → $@"

#───────────────────────────────────────────────────────────────────────────────
# Final FAT12 Floppy Image (1.44 MB)
#───────────────────────────────────────────────────────────────────────────────

IMG            := $(BUILD_DIR)/Quantum.img
IMG_SECTORS    := 2880       # 1.44 MB / 512 B
IMG_BS         := 512
FAT12_LABEL    := QUANTUM

$(IMG): $(BOOT_STAGE1_BIN) $(BOOT_STAGE2_BIN) $(KER32_BIN)
	@mkdir -p $(dir $@)

	@echo "→ Creating blank 1.44 MB image: $@"
	dd if=/dev/zero of=$@ bs=$(IMG_BS) count=$(IMG_SECTORS) conv=notrunc 2> /dev/null

	@echo "→ Formatting as FAT12 (volume label: $(FAT12_LABEL))"
	$(MKFS) -F12 -n $(FAT12_LABEL) $@

	@echo "→ Copying kernel (qkrnl.qx) into FAT12 root directory"
	$(MCOPY) -i $@ $(KER32_BIN) ::/QKRNL.QX

	@echo "→ Installing Stage 1 bootloader (sector 0)"
	dd if=$(BOOT_STAGE1_BIN) of=$@ bs=$(IMG_BS) count=1 conv=notrunc 2> /dev/null

	@echo "→ Installing Stage 2 bootloader (sector 1)"
	dd if=$(BOOT_STAGE2_BIN) of=$@ bs=$(IMG_BS) seek=1 count=1 conv=notrunc 2> /dev/null

	@echo "[OK] Built FAT12 floppy image → $@"

#───────────────────────────────────────────────────────────────────────────────
# Meta‐targets
#───────────────────────────────────────────────────────────────────────────────

.PHONY: all clean
all: $(IMG)

clean:
	@echo "Cleaning $(BUILD_DIR)/ …"
	@rm -rf $(BUILD_DIR)
	@echo "[OK] Removed build/ directory."
