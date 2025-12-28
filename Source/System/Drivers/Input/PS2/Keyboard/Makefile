# Makefile for the PS/2 keyboard driver (user-mode) on IA32
#
# Expects PROJECT_ROOT, BUILD_DIR, ARCH from the top-level Makefile.

PROJECT_ROOT ?= $(abspath ../../../..)
SRC_ROOT     := $(PROJECT_ROOT)/Source
BUILD_DIR    ?= $(PROJECT_ROOT)/Build

ARCH ?= IA32

DRIVER_DIR      := $(SRC_ROOT)/System/Drivers/Input/PS2/Keyboard
CTRL_DIR        := $(SRC_ROOT)/System/Drivers/Input/PS2/Controller
USER_RUNTIME    := $(SRC_ROOT)/System/User/Runtime
LIBQ_DIR        := $(SRC_ROOT)/Libraries/Quantum
LIBQ_INCLUDE    := $(PROJECT_ROOT)/Source/Libraries/Quantum/Include
KEYBOARD_INCLUDE := $(DRIVER_DIR)/Include
CTRL_INCLUDE    := $(CTRL_DIR)/Include

CC32      ?= x86_64-linux-gnu-gcc
CFLAGS32  ?= -fno-pic -fno-pie -m32 -ffreestanding -O2 -Wall -fno-exceptions \
            -fno-rtti -nostdinc -nostdinc++ -fno-stack-protector -fno-builtin \
            -mno-sse -mno-sse2 -mno-mmx -DQUANTUM_ARCH_IA32
LD32      ?= x86_64-linux-gnu-ld
LDFLAGS32 ?= --no-pie -m elf_i386
OBJCOPY32 ?= x86_64-linux-gnu-objcopy

DRIVER_ELF := $(BUILD_DIR)/Drivers/Input/PS2/Keyboard/Keyboard.elf
DRIVER_QX  := $(BUILD_DIR)/Drivers/Input/PS2/Keyboard/Keyboard.qx
LIBQ_LIB := $(BUILD_DIR)/Libraries/Quantum/libQuantum.a

KEYBOARD_SRCS := \
	$(DRIVER_DIR)/Driver.cpp \
	$(DRIVER_DIR)/Main.cpp \
	$(CTRL_DIR)/Controller.cpp \
	$(USER_RUNTIME)/CRT0.cpp \
	$(USER_RUNTIME)/Memory.cpp \
	$(USER_RUNTIME)/Heap.cpp

KEYBOARD_HDRS := \
	$(DRIVER_DIR)/Include/Driver.hpp \
	$(CTRL_DIR)/Include/Controller.hpp

.PHONY: all clean keyboard

all: keyboard

keyboard: $(DRIVER_QX)

$(LIBQ_LIB):
	$(MAKE) -C $(LIBQ_DIR) BUILD_DIR=$(BUILD_DIR) PROJECT_ROOT=$(PROJECT_ROOT) \
		ARCH=$(ARCH) lib

$(DRIVER_ELF): $(KEYBOARD_SRCS) $(KEYBOARD_HDRS) $(DRIVER_DIR)/Link.ld $(LIBQ_LIB)
	@mkdir -p $(dir $@)
	$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-I$(LIBQ_INCLUDE) -I$(KEYBOARD_INCLUDE) -I$(CTRL_INCLUDE) \
		-T $(DRIVER_DIR)/Link.ld \
		$(KEYBOARD_SRCS) $(LIBQ_LIB) -o $@
	@echo "[OK] Linked Keyboard.elf -> $@"

$(DRIVER_QX): $(DRIVER_ELF)
	$(OBJCOPY32) -O binary $< $@
	@echo "[OK] Created Keyboard.qx -> $@"

clean:
	@rm -rf $(BUILD_DIR)/Drivers/Input/PS2/Keyboard
