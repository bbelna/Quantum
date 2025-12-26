# Makefile for the FAT12 file system service (user-mode) on IA32
#
# Expects PROJECT_ROOT, BUILD_DIR, ARCH from the top-level Makefile.

PROJECT_ROOT ?= $(abspath ../../../..)
SRC_ROOT     := $(PROJECT_ROOT)/Source
BUILD_DIR    ?= $(PROJECT_ROOT)/Build

ARCH ?= IA32

FAT12_DIR     := $(SRC_ROOT)/System/FileSystems/FAT12
USER_RUNTIME  := $(SRC_ROOT)/System/User/Runtime
LIBQ_INCLUDE  := $(PROJECT_ROOT)/Source/Libraries/Quantum/Include
FAT12_INCLUDE := $(FAT12_DIR)/Include

CC32      ?= x86_64-linux-gnu-gcc
CFLAGS32  ?= -fno-pic -fno-pie -m32 -ffreestanding -O2 -Wall -fno-exceptions \
            -fno-rtti -nostdinc -nostdinc++ -fno-stack-protector -fno-builtin \
            -mno-sse -mno-sse2 -mno-mmx -DQUANTUM_ARCH_IA32
LD32      ?= x86_64-linux-gnu-ld
LDFLAGS32 ?= --no-pie -m elf_i386
OBJCOPY32 ?= x86_64-linux-gnu-objcopy

SERVICE_ELF := $(BUILD_DIR)/FileSystems/FAT12/FAT12.elf
SERVICE_QX  := $(BUILD_DIR)/FileSystems/FAT12/fat12.qx

FAT12_SRCS := \
	$(FAT12_DIR)/Service.cpp \
	$(FAT12_DIR)/FAT.cpp \
	$(FAT12_DIR)/Directory.cpp \
	$(FAT12_DIR)/File.cpp \
	$(FAT12_DIR)/Volume.cpp \
	$(FAT12_DIR)/Tests.cpp \
	$(FAT12_DIR)/Main.cpp \
	$(USER_RUNTIME)/CRT0.cpp \
	$(USER_RUNTIME)/Memory.cpp \
	$(USER_RUNTIME)/Heap.cpp

FAT12_HDRS := \
	$(FAT12_DIR)/Include/Tests.hpp

.PHONY: all clean fat12

all: fat12

fat12: $(SERVICE_QX)

$(SERVICE_ELF): $(FAT12_SRCS) $(FAT12_HDRS) $(FAT12_DIR)/Link.ld
	@mkdir -p $(dir $@)
	$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-I$(LIBQ_INCLUDE) -I$(FAT12_INCLUDE) \
		-T $(FAT12_DIR)/Link.ld \
		$(FAT12_SRCS) -o $@
	@echo "[OK] Linked FAT12.elf -> $@"

$(SERVICE_QX): $(SERVICE_ELF)
	$(OBJCOPY32) -O binary $< $@
	@echo "[OK] Created fat12.qx -> $@"

clean:
	@rm -rf $(BUILD_DIR)/FileSystems/FAT12
