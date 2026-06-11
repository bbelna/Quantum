# Makefile for Echo utility

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

ECHO_BIN := $(BUILD)/echo.qx
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean Echo

all: Echo

-include $(OBJS:.o=.d)

Echo: $(ECHO_BIN)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Commands.Echo)
	@$(CC32) $(CFLAGS32) -I. -I$(PROJECT_INCLUDE) -c $< -o $@

$(ECHO_BIN): $(OBJS) $(CORE) $(THREADING) $(STREAMING) $(RUNTIME)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Commands.Echo)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(USER_LINKER) \
		$(OBJS) $(STREAMING) $(CORE) $(THREADING) $(RUNTIME) -o $@.elf
	$(call ECHO,QXF $(REL)/$@,Quantum.Commands.Echo)
	@$(QXF_PACK) $@.elf -o $@ --type qx

clean:
	$(call ECHO,RM $(BUILD),Quantum.Commands.Echo)
	@rm -rf $(BUILD)
