# Makefile for the floppy driver (user-mode) on IA32
#
# Expects PROJECT_ROOT, BUILD_DIR, ARCH from the top-level Makefile.

PROJECT_ROOT ?= $(abspath ../../../..)
SRC_ROOT     := $(PROJECT_ROOT)/Source
BUILD_DIR    ?= $(PROJECT_ROOT)/Build

ARCH ?= IA32

DRIVER_DIR    := $(SRC_ROOT)/System/Drivers/Storage/Floppy
LIBQ_INCLUDE  := $(PROJECT_ROOT)/Source/Libraries/Quantum/Include

CC32      ?= x86_64-linux-gnu-gcc
CFLAGS32  ?= -fno-pic -fno-pie -m32 -ffreestanding -O2 -Wall -fno-exceptions \
            -fno-rtti -nostdinc -nostdinc++ -fno-stack-protector -fno-builtin \
            -mno-sse -mno-sse2 -mno-mmx -DQUANTUM_ARCH_IA32
LD32      ?= x86_64-linux-gnu-ld
LDFLAGS32 ?= --no-pie -m elf_i386
OBJCOPY32 ?= x86_64-linux-gnu-objcopy

DRIVER_ELF := $(BUILD_DIR)/Drivers/Storage/Floppy/Floppy.elf
DRIVER_QX  := $(BUILD_DIR)/Drivers/Storage/Floppy/Floppy.qx

.PHONY: all clean floppy

all: floppy

floppy: $(DRIVER_QX)

$(DRIVER_ELF): $(DRIVER_DIR)/Floppy.cpp $(DRIVER_DIR)/Main.cpp $(DRIVER_DIR)/Floppy.hpp $(DRIVER_DIR)/Link.ld
	@mkdir -p $(dir $@)
	$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-I$(LIBQ_INCLUDE) \
		-T $(DRIVER_DIR)/Link.ld \
		$(DRIVER_DIR)/Floppy.cpp $(DRIVER_DIR)/Main.cpp -o $@
	@echo "[OK] Linked Floppy.elf -> $@"

$(DRIVER_QX): $(DRIVER_ELF)
	$(OBJCOPY32) -O binary $< $@
	@echo "[OK] Created Floppy.qx -> $@"

clean:
	@rm -rf $(BUILD_DIR)/Drivers/Storage/Floppy
