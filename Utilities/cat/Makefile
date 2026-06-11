# Makefile for Cat command

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

CAT_BIN := $(BUILD)/cat.qx
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean Cat

all: Cat

-include $(OBJS:.o=.d)

Cat: $(CAT_BIN)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Commands.Cat)
	@$(CC32) $(CFLAGS32) -I. -I$(PROJECT_INCLUDE) -c $< -o $@

$(CAT_BIN): $(OBJS) $(STREAMING) $(THREADING) $(CORE) $(RUNTIME)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Commands.Cat)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(USER_LINKER) \
		$(OBJS) $(STREAMING) $(THREADING) $(CORE) $(RUNTIME) -o $@.elf
	$(call ECHO,QXF $(REL)/$@,Quantum.Commands.Cat)
	@$(QXF_PACK) $@.elf -o $@ --type qx

clean:
	$(call ECHO,RM $(BUILD),Quantum.Commands.Cat)
	@rm -rf $(BUILD)
