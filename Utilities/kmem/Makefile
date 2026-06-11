# Makefile for KernelMemory command

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

KMEM_BIN := $(BUILD)/kmem.qx
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean KernelMemory

all: KernelMemory

-include $(OBJS:.o=.d)

KernelMemory: $(KMEM_BIN)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Commands.KernelMemory)
	@$(CC32) $(CFLAGS32) -I. -I$(PROJECT_INCLUDE) -c $< -o $@

$(KMEM_BIN): $(OBJS) $(STREAMING) $(THREADING) $(CORE) $(RUNTIME)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Commands.KernelMemory)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(USER_LINKER) \
		$(OBJS) $(STREAMING) $(THREADING) $(CORE) $(RUNTIME) -o $@.elf
	$(call ECHO,QXF $(REL)/$@,Quantum.Commands.KernelMemory)
	@$(QXF_PACK) $@.elf -o $@ --type qx

clean:
	$(call ECHO,RM $(BUILD),Quantum.Commands.KernelMemory)
	@rm -rf $(BUILD)
