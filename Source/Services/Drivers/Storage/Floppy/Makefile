# Makefile for the floppy driver (user-mode) on IA32
#
# Expects PROJECT_ROOT, BUILD_DIR, ARCH from the top-level Makefile.

PROJECT_ROOT ?= $(abspath ../../../..)
SRC_ROOT     := $(PROJECT_ROOT)/Source
BUILD_DIR    ?= $(PROJECT_ROOT)/Build

ARCH ?= IA32

DRIVER_DIR    := $(SRC_ROOT)/Services/Drivers/Storage/Floppy
USER_RUNTIME  := $(SRC_ROOT)/System/User/Runtime
LIBQ_DIR      := $(SRC_ROOT)/Libraries/Quantum
LIBQ_INCLUDE  := $(PROJECT_ROOT)/Source/Libraries/Quantum/Include
FLOPPY_INCLUDE := $(DRIVER_DIR)/Include

CC32      ?= x86_64-linux-gnu-gcc
CFLAGS32  ?= -fno-pic -fno-pie -m32 -ffreestanding -O2 -Wall -fno-exceptions \
            -fno-rtti -nostdinc -nostdinc++ -fno-stack-protector -fno-builtin \
            -mno-sse -mno-sse2 -mno-mmx -DQUANTUM_ARCH_IA32
LD32      ?= x86_64-linux-gnu-ld
LDFLAGS32 ?= --no-pie -m elf_i386
OBJCOPY32 ?= x86_64-linux-gnu-objcopy

DRIVER_ELF := $(BUILD_DIR)/Drivers/Storage/Floppy/Floppy.elf
DRIVER_QX  := $(BUILD_DIR)/Drivers/Storage/Floppy/Floppy.qx
LIBQ_LIB := $(BUILD_DIR)/Libraries/Quantum/libQuantum.a

FLOPPY_SRCS := \
	$(DRIVER_DIR)/Driver.cpp \
	$(DRIVER_DIR)/Main.cpp \
	$(USER_RUNTIME)/CRT0.cpp \
	$(USER_RUNTIME)/Memory.cpp \
	$(USER_RUNTIME)/Heap.cpp

.PHONY: all clean floppy

all: floppy

floppy: $(DRIVER_QX)

$(LIBQ_LIB):
	$(MAKE) -C $(LIBQ_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) \
		ARCH=$(ARCH) lib

$(DRIVER_ELF): $(FLOPPY_SRCS) $(FLOPPY_HDRS) $(DRIVER_DIR)/Link.ld $(LIBQ_LIB)
	@mkdir -p $(dir $@)
	$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-I$(LIBQ_INCLUDE) -I$(FLOPPY_INCLUDE) \
		-T $(DRIVER_DIR)/Link.ld \
		$(FLOPPY_SRCS) $(LIBQ_LIB) -o $@
	@echo "[OK] Linked Floppy.elf -> $@"

$(DRIVER_QX): $(DRIVER_ELF)
	$(OBJCOPY32) -O binary $< $@
	@echo "[OK] Created Floppy.qx -> $@"

clean:
	@rm -rf $(BUILD_DIR)/Drivers/Storage/Floppy
