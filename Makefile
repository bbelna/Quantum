# Makefile (top-level) for Quantum
# Builds a BIOS floppy image (1.44 MB) using configured arch/boot medium.
#
# Usage:
#   make all    # Build/Quantum.img
#   make clean  # remove Build/

.DEFAULT_GOAL := default

# Build configuration overrides
-include Configuration/Makefile.config.mk

PROJECT_ROOT := $(CURDIR)
SRC_ROOT     := $(PROJECT_ROOT)/Source
BUILD_DIR    := $(PROJECT_ROOT)/Build

ARCH        ?= IA32
BOOT_MEDIUM ?= Floppy

BOOT_DIR   := $(SRC_ROOT)/System/Boot/$(ARCH)
KERNEL_DIR := $(SRC_ROOT)/System/Kernel
COORD_DIR  := $(SRC_ROOT)/System/Coordinator
FLOPPY_DIR := $(SRC_ROOT)/System/Drivers/Storage/Floppy
PS2KBD_DIR := $(SRC_ROOT)/System/Drivers/Input/PS2/Keyboard
FAT12_DIR  := $(SRC_ROOT)/System/FileSystems/FAT12
LIBQ_INCLUDE := $(PROJECT_ROOT)/Source/Libraries/Quantum/Include
COORD_INCLUDE := $(COORD_DIR)/Include

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
MMD    ?= mmd

export ASM ASFLAGS CC32 CFLAGS32 LD32 LDFLAGS32 OBJCOPY32 MKFS MCOPY MMD \
	PROJECT_ROOT BUILD_DIR ARCH BOOT_MEDIUM

# Artifacts
BOOT_STAGE1_BIN := $(BUILD_DIR)/Boot/$(ARCH)/$(BOOT_MEDIUM)/Stage1.bin
BOOT_STAGE2_BIN := $(BUILD_DIR)/Boot/$(ARCH)/$(BOOT_MEDIUM)/Stage2.bin
KER_BIN         := $(BUILD_DIR)/Kernel/$(ARCH)/Kernel.qx

IMG         := $(BUILD_DIR)/Quantum.img
IMG_SECTORS := 2880       # 1.44 MB / 512 B
IMG_BS      := 512
FAT12_LABEL := QUANTUM

# INIT bundle
INIT_MANIFEST ?= $(PROJECT_ROOT)/Configuration/InitManifest.json
INIT_BUNDLE   ?= $(BUILD_DIR)/INIT.BND
BUNDLER       ?= python3 $(PROJECT_ROOT)/Tools/Bundle.py
HAS_INIT_MANIFEST := $(wildcard $(INIT_MANIFEST))

.PHONY: default all boot kernel img clean boot-clean kernel-clean

default: clean all

all: $(IMG)

boot: $(BOOT_STAGE1_BIN) $(BOOT_STAGE2_BIN)

kernel: $(KER_BIN)

coordinator: $(COORD_QX)

floppy: $(FLOPPY_QX)

fat12: $(FAT12_QX)

ps2kbd: $(PS2KBD_QX)

# Build INIT.BND if manifest is present
.PHONY: init-bundle
init-bundle: $(COORD_QX) $(FLOPPY_QX) $(FAT12_QX) $(PS2KBD_QX)
ifeq ($(HAS_INIT_MANIFEST),)
	@echo "INIT manifest not found ($(INIT_MANIFEST)); skipping bundle."
else
	@echo "Bundling INIT.BND from $(INIT_MANIFEST)"
	@$(BUNDLER) --manifest "$(INIT_MANIFEST)" --output "$(INIT_BUNDLE)" \
		--base "$(PROJECT_ROOT)"
endif

$(BOOT_STAGE1_BIN):
	$(MAKE) -C $(BOOT_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) \
		ARCH=$(ARCH) BOOT_MEDIUM=$(BOOT_MEDIUM) stage1

$(BOOT_STAGE2_BIN):
	$(MAKE) -C $(BOOT_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) \
		ARCH=$(ARCH) BOOT_MEDIUM=$(BOOT_MEDIUM) stage2

$(KER_BIN):
	$(MAKE) -C $(KERNEL_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) \
		ARCH=$(ARCH) kernel

COORD_QX := $(BUILD_DIR)/Coordinator/Coordinator.qx
FLOPPY_QX := $(BUILD_DIR)/Drivers/Storage/Floppy/Floppy.qx
PS2KBD_QX := $(BUILD_DIR)/Drivers/Input/PS2/Keyboard/Keyboard.qx
FAT12_QX := $(BUILD_DIR)/FileSystems/FAT12/fat12.qx

$(COORD_QX):
	$(MAKE) -C $(COORD_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) \
		ARCH=$(ARCH) coordinator

$(FLOPPY_QX):
	$(MAKE) -C $(FLOPPY_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) \
		ARCH=$(ARCH) floppy

$(PS2KBD_QX):
	$(MAKE) -C $(PS2KBD_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) \
		ARCH=$(ARCH) keyboard

$(FAT12_QX):
	$(MAKE) -C $(FAT12_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) \
		ARCH=$(ARCH) fat12

$(IMG): $(KER_BIN) $(BOOT_STAGE1_BIN) $(BOOT_STAGE2_BIN) $(COORD_QX) \
	$(FLOPPY_QX) $(PS2KBD_QX) $(FAT12_QX) init-bundle
	@mkdir -p $(dir $@)
	@echo "Creating blank 1.44 MB image: $@"
	dd if=/dev/zero of=$@ bs=$(IMG_BS) count=$(IMG_SECTORS) conv=notrunc \
		status=none
	@echo "Formatting as FAT12 (volume label: $(FAT12_LABEL))"
	$(MKFS) -F 12 -R 5 -n $(FAT12_LABEL) $@
	@echo "Copying kernel (KERNEL.QX) into FAT12 root directory"
	$(MCOPY) -i $@ $(KER_BIN) ::/KERNEL.QX
	@echo "Installing Stage 1 bootloader (sector 0)"
	dd if=$(BOOT_STAGE1_BIN) of=$@ bs=$(IMG_BS) seek=0 count=1 conv=notrunc \
		status=none
	@echo "Installing Stage 2 bootloader (sectors 1-4)"
	dd if=$(BOOT_STAGE2_BIN) of=$@ bs=$(IMG_BS) seek=1 count=4 conv=notrunc,sync \
		status=none
	@if [ -f "$(INIT_BUNDLE)" ]; then \
		echo "Copying INIT.BND into FAT12 root directory"; \
		$(MCOPY) -i $@ $(INIT_BUNDLE) ::/INIT.BND; \
	else \
		echo "INIT.BND not present; skipping bundle copy."; \
	fi
	@echo "[OK] Built FAT12 floppy image -> $@"

boot-clean:
	$(MAKE) -C $(BOOT_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) \
		ARCH=$(ARCH) BOOT_MEDIUM=$(BOOT_MEDIUM) clean

kernel-clean:
	$(MAKE) -C $(KERNEL_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) \
		ARCH=$(ARCH) clean

clean: boot-clean kernel-clean
	@echo "Removing $(BUILD_DIR)/"
	@rm -rf $(BUILD_DIR)
	@echo "[OK] Removed build/ directory."
