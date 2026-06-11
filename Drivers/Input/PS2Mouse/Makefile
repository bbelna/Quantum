# Makefile for IA32 PS/2 Mouse

PROJECT_ROOT := $(abspath ../../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

PS2MOUSE := $(BUILD)/PS2Mouse.qx
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean PS2MouseDriver

all: PS2MouseDriver

-include $(OBJS:.o=.d)

PS2MouseDriver: $(PS2MOUSE)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Drivers.Input.PS2Mouse)
	@$(CC32) $(CFLAGS32) -I$(PROJECT_INCLUDE) -I. \
		-c $< -o $@

$(PS2MOUSE): $(OBJS) $(CORE) $(THREADING) $(RUNTIME)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Drivers.Input.PS2Mouse)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(SERVER_LINKER) \
		$(OBJS) $(CORE) $(THREADING) $(RUNTIME) $(CORE) $(THREADING) -o $@

clean:
	$(call ECHO,RM $(BUILD),Quantum.Drivers.Input.PS2Mouse)
	@rm -rf $(BUILD)
