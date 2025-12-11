# Makefile (top‐level) for Quantum
# Builds a two‐stage FAT12 floppy image (1.44 MB) containing:
#   • Sector 0: Boot.asm (Stage 1)
#   • Sector 1: Floppy.asm (Stage 2 FAT12 loader)
#   • Root directory file: QKRNL.QX (the kernel)
#
# All intermediates and outputs go under build/. Source remains untouched.
#
# Usage:
#   make all    # → build/Quantum.img (FAT12 floppy)
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
KERNEL_ARCH32  := $(KERNEL_SRC_DIR)/Arch/IA32
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
            -fno-rtti -nostdinc -nostdinc++ -fno-stack-protector -fno-builtin \
            -mno-sse -mno-sse2 -mno-mmx -DQUANTUM_ARCH_IA32
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
# Kernel Common Source Files
#───────────────────────────────────────────────────────────────────────────────

KER_COMMON_SRCS := \
	$(wildcard $(KERNEL_COMMON)/*.cpp) \
	$(wildcard $(KERNEL_COMMON)/Drivers/*.cpp) \
	$(wildcard $(KERNEL_COMMON)/Helpers/*.cpp) \
	$(wildcard $(KERNEL_COMMON)/Runtime/*.cpp) \

#───────────────────────────────────────────────────────────────────────────────
# Kernel (IA32) Build
#───────────────────────────────────────────────────────────────────────────────

KER32_OBJ_DIR := $(BUILD_DIR)/Kernel/IA32

GDT_SRC := $(KERNEL_ARCH32)/GDT.asm
GDT_OBJ := $(KER32_OBJ_DIR)/GDT.o

KERNEL_ARCH32_CPP := \
	$(wildcard $(KERNEL_ARCH32)/**/*.cpp) \
	$(wildcard $(KERNEL_ARCH32)/*.cpp)

KERNEL_ARCH32_OBJS := \
	$(patsubst $(KERNEL_SRC_DIR)/%.cpp,$(KER32_OBJ_DIR)/%.cpp.o,$(KERNEL_ARCH32_CPP))


KERNEL_ARCH32_ASM := \
	$(wildcard $(KERNEL_ARCH32)/**/*.asm) \
	$(wildcard $(KERNEL_ARCH32)/*.asm)
KERNEL_ARCH32_ASM_OBJS := \
	$(patsubst $(KERNEL_SRC_DIR)/%.asm,$(KER32_OBJ_DIR)/%.asm.o,$(KERNEL_ARCH32_ASM))

KER32_OBJS := \
	$(patsubst $(KERNEL_COMMON)/%.cpp,$(KER32_OBJ_DIR)/%.cpp.o,$(KER_COMMON_SRCS)) \
	$(KERNEL_ARCH32_OBJS) \
	$(KERNEL_ARCH32_ASM_OBJS)

KER32_ELF     := $(KER32_OBJ_DIR)/qkrnl.elf
KER32_BIN     := $(KER32_OBJ_DIR)/qkrnl.qx

# compile ANY .cpp under src/System/Kernel into build/Kernel/IA32
$(KER32_OBJ_DIR)/%.cpp.o: $(KERNEL_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC32) $(CFLAGS32) -I$(KERNEL_INCLUDE) -c $< -o $@

# assemble ANY .asm under src/System/Kernel into build/Kernel/IA32
$(KER32_OBJ_DIR)/%.asm.o: $(KERNEL_SRC_DIR)/%.asm
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

$(IMG): $(KER32_BIN) $(BOOT_STAGE1_BIN) $(BOOT_STAGE2_BIN)
	@mkdir -p $(dir $@)

	@echo "→ Creating blank 1.44 MB image: $@"
	dd if=/dev/zero of=$@ bs=$(IMG_BS) count=$(IMG_SECTORS) conv=notrunc status=none

	@echo "→ Formatting as FAT12 (volume label: $(FAT12_LABEL))"
	$(MKFS) -F 12 -R 5 -n $(FAT12_LABEL) $@

	@echo "→ Copying kernel (qkrnl.qx) into FAT12 root directory"
	$(MCOPY) -i $@ $(KER32_BIN) ::/QKRNL.QX

	@echo "→ Installing Stage 1 bootloader (sector 0)"
	dd if=$(BOOT_STAGE1_BIN) of=$@ bs=$(IMG_BS) seek=0 count=1 conv=notrunc status=none

	@echo "→ Installing Stage 2 bootloader (sectors 1-2)"
	dd if=$(BOOT_STAGE2_BIN) of=$@ bs=$(IMG_BS) seek=1 count=4 conv=notrunc,sync status=none

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
