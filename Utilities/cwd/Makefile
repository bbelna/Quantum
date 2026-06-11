# Makefile for CurrentWorkingDirectory command

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

CWD_BIN := $(BUILD)/cwd.qx
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean CurrentWorkingDirectory

all: CurrentWorkingDirectory

-include $(OBJS:.o=.d)

CurrentWorkingDirectory: $(CWD_BIN)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Commands.CurrentWorkingDirectory)
	@$(CC32) $(CFLAGS32) -I. -I$(PROJECT_INCLUDE) -c $< -o $@

$(CWD_BIN): $(OBJS) $(CORE) $(THREADING) $(STREAMING) $(RUNTIME) $(CLIENTS)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Commands.CurrentWorkingDirectory)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(USER_LINKER) \
		$(OBJS) $(STREAMING) $(THREADING) $(CORE) $(RUNTIME) -o $@.elf
	$(call ECHO,QXF $(REL)/$@,Quantum.Commands.CurrentWorkingDirectory)
	@$(QXF_PACK) $@.elf -o $@ --type qx

clean:
	$(call ECHO,RM $(BUILD),Quantum.Commands.CurrentWorkingDirectory)
	@rm -rf $(BUILD)
