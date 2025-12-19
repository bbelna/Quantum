# Makefile for IA32 coordinator build
#
# Expects PROJECT_ROOT, BUILD_DIR, ARCH from the top-level Makefile.

PROJECT_ROOT ?= $(abspath ../../..)
SRC_ROOT     := $(PROJECT_ROOT)/Source
BUILD_DIR    ?= $(PROJECT_ROOT)/Build

ARCH ?= IA32

COORD_DIR     := $(SRC_ROOT)/System/Coordinator
USER_RUNTIME  := $(SRC_ROOT)/System/User/Runtime
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

.PHONY: all clean coordinator

all: coordinator

coordinator: $(COORD_QX)

$(COORD_ELF): $(COORD_DIR)/Coordinator.cpp $(COORD_DIR)/Main.cpp $(USER_RUNTIME)/CRT0.cpp $(COORD_DIR)/Include/Coordinator.hpp $(COORD_DIR)/Link.ld
	@mkdir -p $(dir $@)
	$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-I$(LIBQ_INCLUDE) -I$(COORD_INCLUDE) \
		-T $(COORD_DIR)/Link.ld \
		$(COORD_DIR)/Coordinator.cpp $(COORD_DIR)/Main.cpp $(USER_RUNTIME)/CRT0.cpp -o $@
	@echo "[OK] Linked Coordinator.elf -> $@"

$(COORD_QX): $(COORD_ELF)
	$(OBJCOPY32) -O binary $< $@
	@echo "[OK] Created Coordinator.qx -> $@"

clean:
	@rm -rf $(BUILD_DIR)/Coordinator
