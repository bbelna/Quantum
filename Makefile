# Makefile (top-level) for Quantum
# Builds a BIOS floppy image (1.44 MB) using configured arch/boot medium.
#
# Usage:
#   make all    # Build/Quantum.img
#   make clean  # remove Build/

.DEFAULT_GOAL := default

# Optional user overrides
-include Makefile.config.mk

PROJECT_ROOT := $(CURDIR)
SRC_ROOT     := $(PROJECT_ROOT)/Source
BUILD_DIR    := $(PROJECT_ROOT)/Build

ARCH        ?= IA32
BOOT_MEDIUM ?= Floppy

BOOT_DIR   := $(SRC_ROOT)/System/Boot/$(ARCH)
KERNEL_DIR := $(SRC_ROOT)/System/Kernel

# Toolchain defaults (can be overridden by environment)
ASM       ?= nasm
ASFLAGS   ?= -f bin
CC32      ?= x86_64-linux-gnu-gcc
CFLAGS32  ?= -fno-pic -fno-pie -m32 -ffreestanding -O2 -Wall -fno-exceptions \
             -fno-rtti -nostdinc -nostdinc++ -fno-stack-protector -fno-builtin \
             -mno-sse -mno-sse2 -mno-mmx -DQUANTUM_ARCH_IA32
LD32      ?= x86_64-linux-gnu-ld
LDFLAGS32 ?= --no-pie -m elf_i386
OBJCOPY32 ?= x86_64-linux-gnu-objcopy

# Utilities for FAT12 image creation (mtools)
MKFS   ?= mkfs.vfat
MCOPY  ?= mcopy

export ASM ASFLAGS CC32 CFLAGS32 LD32 LDFLAGS32 OBJCOPY32 MKFS MCOPY PROJECT_ROOT BUILD_DIR ARCH BOOT_MEDIUM

# Artifacts
BOOT_STAGE1_BIN := $(BUILD_DIR)/Boot/$(ARCH)/$(BOOT_MEDIUM)/Stage1.bin
BOOT_STAGE2_BIN := $(BUILD_DIR)/Boot/$(ARCH)/$(BOOT_MEDIUM)/Stage2.bin
KER_BIN         := $(BUILD_DIR)/Kernel/$(ARCH)/Kernel.qx

IMG         := $(BUILD_DIR)/Quantum.img
IMG_SECTORS := 2880       # 1.44 MB / 512 B
IMG_BS      := 512
FAT12_LABEL := QUANTUM

.PHONY: default all boot kernel img clean boot-clean kernel-clean

default: clean all

all: $(IMG)

boot: $(BOOT_STAGE1_BIN) $(BOOT_STAGE2_BIN)

kernel: $(KER_BIN)

$(BOOT_STAGE1_BIN):
	$(MAKE) -C $(BOOT_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) ARCH=$(ARCH) BOOT_MEDIUM=$(BOOT_MEDIUM) stage1

$(BOOT_STAGE2_BIN):
	$(MAKE) -C $(BOOT_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) ARCH=$(ARCH) BOOT_MEDIUM=$(BOOT_MEDIUM) stage2

$(KER_BIN):
	$(MAKE) -C $(KERNEL_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) ARCH=$(ARCH) kernel

$(IMG): $(KER_BIN) $(BOOT_STAGE1_BIN) $(BOOT_STAGE2_BIN)
	@mkdir -p $(dir $@)
	@echo "Creating blank 1.44 MB image: $@"
	dd if=/dev/zero of=$@ bs=$(IMG_BS) count=$(IMG_SECTORS) conv=notrunc status=none
	@echo "Formatting as FAT12 (volume label: $(FAT12_LABEL))"
	$(MKFS) -F 12 -R 5 -n $(FAT12_LABEL) $@
	@echo "Copying kernel (KERNEL.QX) into FAT12 root directory"
	$(MCOPY) -i $@ $(KER_BIN) ::/KERNEL.QX
	@echo "Installing Stage 1 bootloader (sector 0)"
	dd if=$(BOOT_STAGE1_BIN) of=$@ bs=$(IMG_BS) seek=0 count=1 conv=notrunc status=none
	@echo "Installing Stage 2 bootloader (sectors 1-4)"
	dd if=$(BOOT_STAGE2_BIN) of=$@ bs=$(IMG_BS) seek=1 count=4 conv=notrunc,sync status=none
	@echo "[OK] Built FAT12 floppy image -> $@"

boot-clean:
	$(MAKE) -C $(BOOT_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) ARCH=$(ARCH) BOOT_MEDIUM=$(BOOT_MEDIUM) clean

kernel-clean:
	$(MAKE) -C $(KERNEL_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) ARCH=$(ARCH) clean

clean: boot-clean kernel-clean
	@echo "Removing $(BUILD_DIR)/"
	@rm -rf $(BUILD_DIR)
	@echo "[OK] Removed build/ directory."
