# Makefile for IA32 Shell

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

SHELL_BIN := $(BUILD)/Terminal.qapp
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean Shell

all: Shell

-include $(OBJS:.o=.d)

Shell: $(SHELL_BIN)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Apps.Terminal)
	@$(CC32) $(CFLAGS32) -I. -I$(PROJECT_INCLUDE) -c $< -o $@

$(SHELL_BIN): $(OBJS) $(CORE) $(THREADING) $(FONTS) $(APP_LIB) $(COMPONENTS) $(MENUS) $(UI) $(CLIENTS) $(STREAMING) $(RUNTIME)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Apps.Terminal)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(USER_LINKER) \
		$(OBJS) $(APP_LIB) $(COMPONENTS) $(MENUS) $(UI) $(CLIENTS) $(STREAMING) $(CORE) $(THREADING) $(FONTS) $(RUNTIME) -o $@.elf
	$(call ECHO,QXF $(REL)/$@,Quantum.Apps.Terminal)
	@$(QXF_PACK) $@.elf -o $@ --type qapp

clean:
	$(call ECHO,RM $(BUILD),Quantum.Apps.Terminal)
	@rm -rf $(BUILD)
