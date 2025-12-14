# Makefile for IA32 kernel build
#
# Expects PROJECT_ROOT, BUILD_DIR, ARCH from the top-level Makefile.

PROJECT_ROOT ?= $(abspath ../../..)
SRC_ROOT     := $(PROJECT_ROOT)/Source
BUILD_DIR    ?= $(PROJECT_ROOT)/Build

ARCH ?= IA32

KERNEL_SRC_DIR := $(SRC_ROOT)/System/Kernel
KERNEL_ARCH    := $(KERNEL_SRC_DIR)/Arch/$(ARCH)
KERNEL_INCLUDE := $(KERNEL_SRC_DIR)/Include

ASM     ?= nasm
CC32    ?= x86_64-linux-gnu-gcc
CFLAGS32 ?= -fno-pic -fno-pie -m32 -ffreestanding -O2 -Wall -fno-exceptions \
            -fno-rtti -nostdinc -nostdinc++ -fno-stack-protector -fno-builtin \
            -mno-sse -mno-sse2 -mno-mmx -DQUANTUM_ARCH_IA32
LD32      ?= x86_64-linux-gnu-ld
LDFLAGS32 ?= --no-pie -m elf_i386
OBJCOPY32 ?= x86_64-linux-gnu-objcopy

KER_COMMON_SRCS := \
	$(wildcard $(KERNEL_SRC_DIR)/*.cpp) \
	$(wildcard $(KERNEL_SRC_DIR)/Drivers/*.cpp) \
	$(wildcard $(KERNEL_SRC_DIR)/Helpers/*.cpp) \
	$(wildcard $(KERNEL_SRC_DIR)/Runtime/*.cpp)

KERNEL_ARCH_CPP := \
	$(wildcard $(KERNEL_ARCH)/**/*.cpp) \
	$(wildcard $(KERNEL_ARCH)/*.cpp)

KERNEL_ARCH_ASM := \
	$(wildcard $(KERNEL_ARCH)/**/*.asm) \
	$(wildcard $(KERNEL_ARCH)/*.asm)

KER_OBJ_DIR := $(BUILD_DIR)/Kernel/$(ARCH)
KERNEL_ARCH_OBJS := \
	$(patsubst $(KERNEL_SRC_DIR)/%.cpp,$(KER_OBJ_DIR)/%.cpp.o,$(KERNEL_ARCH_CPP))
KERNEL_ARCH_ASM_OBJS := \
	$(patsubst $(KERNEL_SRC_DIR)/%.asm,$(KER_OBJ_DIR)/%.asm.o,$(KERNEL_ARCH_ASM))

KER_OBJS := \
	$(patsubst $(KERNEL_SRC_DIR)/%.cpp,$(KER_OBJ_DIR)/%.cpp.o,$(KER_COMMON_SRCS)) \
	$(KERNEL_ARCH_OBJS) \
	$(KERNEL_ARCH_ASM_OBJS)

KER_ELF := $(KER_OBJ_DIR)/Kernel.elf
KER_BIN := $(KER_OBJ_DIR)/Kernel.qx

.PHONY: all clean kernel

all: kernel

kernel: $(KER_BIN)

$(KER_OBJ_DIR)/%.cpp.o: $(KERNEL_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC32) $(CFLAGS32) -I$(KERNEL_INCLUDE) -c $< -o $@

$(KER_OBJ_DIR)/%.asm.o: $(KERNEL_SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(ASM) -f elf32 $< -o $@

$(KER_ELF): $(KER_OBJS) $(KERNEL_ARCH)/Link.ld
	@mkdir -p $(dir $@)
	$(CC32) $(CFLAGS32) -m32 -nostdlib -static -Wl,--no-pie \
		-T $(KERNEL_ARCH)/Link.ld \
		$(KER_OBJS) -o $@
	@echo "[OK] Linked Kernel.elf -> $@"

$(KER_BIN): $(KER_ELF)
	$(OBJCOPY32) -O binary $< $@
	@echo "[OK] Created Kernel.qx -> $@"
clean:
	@rm -rf $(KER_OBJ_DIR)
