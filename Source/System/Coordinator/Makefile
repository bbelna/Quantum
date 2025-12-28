# Makefile for IA32 coordinator build
#
# Expects PROJECT_ROOT, BUILD_DIR, ARCH from the top-level Makefile.

PROJECT_ROOT ?= $(abspath ../../..)
SRC_ROOT     := $(PROJECT_ROOT)/Source
BUILD_DIR    ?= $(PROJECT_ROOT)/Build

ARCH ?= IA32

COORD_DIR     := $(SRC_ROOT)/System/Coordinator
USER_RUNTIME  := $(SRC_ROOT)/System/User/Runtime
LIBQ_DIR      := $(SRC_ROOT)/Libraries/Quantum
LIBQ_INCLUDE  := $(PROJECT_ROOT)/Source/Libraries/Quantum/Include
COORD_INCLUDE := $(COORD_DIR)/Include

CC32      ?= x86_64-linux-gnu-gcc
CFLAGS32  ?= -fno-pic -fno-pie -m32 -ffreestanding -O2 -Wall -fno-exceptions \
            -fno-rtti -nostdinc -nostdinc++ -fno-stack-protector -fno-builtin \
            -mno-sse -mno-sse2 -mno-mmx -DQUANTUM_ARCH_IA32
LD32      ?= x86_64-linux-gnu-ld
LDFLAGS32 ?= --no-pie -m elf_i386
OBJCOPY32 ?= x86_64-linux-gnu-objcopy

COORD_ELF := $(BUILD_DIR)/Coordinator/Coordinator.elf
COORD_QX  := $(BUILD_DIR)/Coordinator/Coordinator.qx
LIBQ_LIB := $(BUILD_DIR)/Libraries/Quantum/libQuantum.a

COORD_SRCS := \
	$(COORD_DIR)/Application.cpp \
	$(COORD_DIR)/Main.cpp \
	$(COORD_DIR)/IRQ.cpp \
	$(COORD_DIR)/FileSystem.cpp \
	$(COORD_DIR)/Input.cpp \
	$(USER_RUNTIME)/CRT0.cpp \
	$(USER_RUNTIME)/Memory.cpp \
	$(USER_RUNTIME)/Heap.cpp

COORD_HDRS := \
	$(COORD_DIR)/Include/Application.hpp \
	$(COORD_DIR)/Include/IRQ.hpp \
	$(COORD_DIR)/Include/FileSystem.hpp \
	$(COORD_DIR)/Include/Input.hpp

.PHONY: all clean coordinator

all: coordinator

coordinator: $(COORD_QX)

$(LIBQ_LIB):
	$(MAKE) -C $(LIBQ_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) \
		ARCH=$(ARCH) lib

$(COORD_ELF): $(COORD_SRCS) $(COORD_HDRS) $(COORD_DIR)/Link.ld $(LIBQ_LIB)
	@mkdir -p $(dir $@)
	$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-I$(LIBQ_INCLUDE) -I$(COORD_INCLUDE) \
		-T $(COORD_DIR)/Link.ld \
		$(COORD_SRCS) $(LIBQ_LIB) -o $@
	@echo "[OK] Linked Coordinator.elf -> $@"

$(COORD_QX): $(COORD_ELF)
	$(OBJCOPY32) -O binary $< $@
	@echo "[OK] Created Coordinator.qx -> $@"

clean:
	@rm -rf $(BUILD_DIR)/Coordinator
