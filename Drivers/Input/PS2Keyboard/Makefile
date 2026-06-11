# Makefile for IA32 PS/2

PROJECT_ROOT := $(abspath ../../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

PS2KEYBOARD := $(BUILD)/PS2Keyboard.qx
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean PS2Keyboard

all: PS2Keyboard

-include $(OBJS:.o=.d)

PS2Keyboard: $(PS2KEYBOARD)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Drivers.Input.PS2Keyboard)
	@$(CC32) $(CFLAGS32) -I$(PROJECT_INCLUDE) -I. \
		-c $< -o $@

$(PS2KEYBOARD): $(OBJS) $(CORE) $(THREADING) $(RUNTIME)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Drivers.Input.PS2Keyboard)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(SERVER_LINKER) \
		$(OBJS) $(CORE) $(THREADING) $(RUNTIME) $(CORE) $(THREADING) -o $@

clean:
	$(call ECHO,RM $(BUILD),Quantum.Drivers.Input.PS2Keyboard)
	@rm -rf $(BUILD)
