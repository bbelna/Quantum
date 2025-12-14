# Makefile for IA32 BIOS boot stages (floppy)
#
# Expects PROJECT_ROOT, BUILD_DIR, ARCH, BOOT_MEDIUM from the top-level Makefile.
# Defaults allow standalone invocation from this directory.

PROJECT_ROOT ?= $(abspath ../../../..)
SRC_ROOT     := $(PROJECT_ROOT)/Source
BUILD_DIR    ?= $(PROJECT_ROOT)/Build

ARCH        ?= IA32
BOOT_MEDIUM ?= Floppy

ASM     ?= nasm
ASFLAGS ?= -f bin
ASFLAGS += -I$(SRC_ROOT)/System/Boot/$(ARCH)

STAGE1_SRC := $(BOOT_MEDIUM)/Stage1.asm
STAGE2_SRC := $(BOOT_MEDIUM)/Stage2.asm

OUT_DIR      := $(BUILD_DIR)/Boot/$(ARCH)/$(BOOT_MEDIUM)
STAGE1_BIN   := $(OUT_DIR)/Stage1.bin
STAGE2_BIN   := $(OUT_DIR)/Stage2.bin

.PHONY: all clean stage1 stage2

all: stage1 stage2

stage1: $(STAGE1_BIN)

stage2: $(STAGE2_BIN)

$(STAGE1_BIN): $(STAGE1_SRC) $(SRC_ROOT)/System/Boot/IA32/Constants.inc $(SRC_ROOT)/System/Boot/IA32/Console.inc
	@mkdir -p $(dir $@)
	$(ASM) $(ASFLAGS) $< -o $@
	@echo "[OK] Assembled IA32 Stage1 -> $@"

$(STAGE2_BIN): $(STAGE2_SRC) $(SRC_ROOT)/System/Boot/IA32/Constants.inc $(SRC_ROOT)/System/Boot/IA32/Console.inc
	@mkdir -p $(dir $@)
	$(ASM) $(ASFLAGS) $< -o $@
	@echo "[OK] Assembled IA32 Stage2 -> $@"

clean:
	@rm -rf $(BUILD_DIR)/Boot/IA32
