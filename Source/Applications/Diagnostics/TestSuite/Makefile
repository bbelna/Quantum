# Makefile for the TestSuite application (user-mode) on IA32
#
# Expects PROJECT_ROOT, BUILD_DIR, ARCH from the top-level Makefile.

PROJECT_ROOT ?= $(abspath ../../../..)
SRC_ROOT     := $(PROJECT_ROOT)/Source
BUILD_DIR    ?= $(PROJECT_ROOT)/Build

ARCH ?= IA32

APP_DIR       := $(SRC_ROOT)/Applications/Diagnostics/TestSuite
USER_RUNTIME  := $(SRC_ROOT)/System/User/Runtime
LIBQ_DIR      := $(SRC_ROOT)/Libraries/Quantum
LIBQ_INCLUDE  := $(PROJECT_ROOT)/Source/Libraries/Quantum/Include
APP_INCLUDE   := $(APP_DIR)/Include

CC32      ?= x86_64-linux-gnu-gcc
CFLAGS32  ?= -fno-pic -fno-pie -m32 -ffreestanding -O2 -Wall -fno-exceptions \
            -fno-rtti -nostdinc -nostdinc++ -fno-stack-protector -fno-builtin \
            -mno-sse -mno-sse2 -mno-mmx -DQUANTUM_ARCH_IA32
LD32      ?= x86_64-linux-gnu-ld
LDFLAGS32 ?= --no-pie -m elf_i386
OBJCOPY32 ?= x86_64-linux-gnu-objcopy

APP_ELF := $(BUILD_DIR)/Applications/Diagnostics/TestSuite/TestSuite.elf
APP_QX  := $(BUILD_DIR)/Applications/Diagnostics/TestSuite/TestSuite.qx
LIBQ_LIB := $(BUILD_DIR)/Libraries/Quantum/libQuantum.a

APP_SRCS := \
	$(APP_DIR)/Main.cpp \
	$(APP_DIR)/Testing.cpp \
	$(APP_DIR)/Tests/FloppyTests.cpp \
	$(APP_DIR)/Tests/FAT12Tests.cpp \
	$(APP_DIR)/Tests/IPCTests.cpp \
	$(APP_DIR)/Tests/InputTests.cpp \
	$(USER_RUNTIME)/CRT0.cpp \
	$(USER_RUNTIME)/Memory.cpp \
	$(USER_RUNTIME)/Heap.cpp

APP_HDRS := \
	$(APP_DIR)/Include/Testing.hpp \
	$(APP_DIR)/Include/Tests/FloppyTests.hpp \
	$(APP_DIR)/Include/Tests/FAT12Tests.hpp \
	$(APP_DIR)/Include/Tests/IPCTests.hpp \
	$(APP_DIR)/Include/Tests/InputTests.hpp

.PHONY: all clean testsuite

all: testsuite

testsuite: $(APP_QX)

$(LIBQ_LIB):
	$(MAKE) -C $(LIBQ_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) \
		ARCH=$(ARCH) lib

$(APP_ELF): $(APP_SRCS) $(APP_HDRS) $(APP_DIR)/Link.ld $(LIBQ_LIB)
	@mkdir -p $(dir $@)
	$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-I$(LIBQ_INCLUDE) -I$(APP_INCLUDE) \
		-T $(APP_DIR)/Link.ld \
		$(APP_SRCS) $(LIBQ_LIB) -o $@
	@echo "[OK] Linked TestSuite.elf -> $@"

$(APP_QX): $(APP_ELF)
	$(OBJCOPY32) -O binary $< $@
	@echo "[OK] Created TestSuite.qx -> $@"

clean:
	@rm -rf $(BUILD_DIR)/Applications/Diagnostics/TestSuite
