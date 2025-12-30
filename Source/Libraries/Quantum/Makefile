# Makefile for Quantum support library
#
# Expects PROJECT_ROOT, BUILD_DIR, ARCH from the top-level Makefile.

PROJECT_ROOT ?= $(abspath ../..)
SRC_ROOT     := $(PROJECT_ROOT)/Source
BUILD_DIR    ?= $(PROJECT_ROOT)/Build

ARCH ?= IA32

LIBQ_DIR     := $(SRC_ROOT)/Libraries/Quantum
LIBQ_INCLUDE := $(LIBQ_DIR)/Include

CC32      ?= x86_64-linux-gnu-gcc
CFLAGS32  ?= -fno-pic -fno-pie -m32 -ffreestanding -O2 -Wall -fno-exceptions \
            -fno-rtti -nostdinc -nostdinc++ -fno-stack-protector -fno-builtin \
            -mno-sse -mno-sse2 -mno-mmx -DQUANTUM_ARCH_IA32
AR32      ?= x86_64-linux-gnu-ar
ARFLAGS   ?= rcs

LIBQ_OBJ_DIR := $(BUILD_DIR)/Libraries/Quantum
LIBQ_LIB     := $(LIBQ_OBJ_DIR)/libQuantum.a

LIBQ_SRCS := \
	$(LIBQ_DIR)/CString.cpp \
	$(LIBQ_DIR)/Align.cpp \
	$(LIBQ_DIR)/Debug.cpp \
	$(LIBQ_DIR)/Bytes.cpp

LIBQ_OBJS := \
	$(patsubst $(LIBQ_DIR)/%.cpp,$(LIBQ_OBJ_DIR)/%.cpp.o,$(LIBQ_SRCS))

.PHONY: all clean lib

all: lib

lib: $(LIBQ_LIB)

$(LIBQ_OBJ_DIR)/%.cpp.o: $(LIBQ_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC32) $(CFLAGS32) -I$(LIBQ_INCLUDE) -c $< -o $@

$(LIBQ_LIB): $(LIBQ_OBJS)
	@mkdir -p $(dir $@)
	$(AR32) $(ARFLAGS) $@ $(LIBQ_OBJS)
	@echo "[OK] Archived libQuantum.a -> $@"

clean:
	@rm -rf $(LIBQ_OBJ_DIR)
