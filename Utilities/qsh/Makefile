# Makefile for Quantum OS Text-Mode Shell

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

SHELL_BIN := $(BUILD)/qsh.qx
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean Shell

all: Shell

-include $(OBJS:.o=.d)

Shell: $(SHELL_BIN)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Shell)
	@$(CC32) $(CFLAGS32) -I. -I$(PROJECT_INCLUDE) -c $< -o $@

$(SHELL_BIN): $(OBJS) $(CLIENTS) $(STREAMING) $(CORE) $(THREADING) $(RUNTIME)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Shell)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(USER_LINKER) \
		$(OBJS) $(CLIENTS) $(STREAMING) $(CORE) $(THREADING) $(RUNTIME) -o $@.elf
	$(call ECHO,QXF $(REL)/$@,Quantum.Shell)
	@$(QXF_PACK) $@.elf -o $@ --type qx

clean:
	$(call ECHO,RM $(BUILD),Quantum.Shell)
	@rm -rf $(BUILD)
