# Makefile for IA32 Startup

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

STARTUP_BIN := $(STARTUP)
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean Startup

all: Startup

-include $(OBJS:.o=.d)

Startup: $(STARTUP_BIN)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Servers.Startup)
	@$(CC32) $(CFLAGS32) -I$(PROJECT_INCLUDE) -I. \
		-c $< -o $@

$(STARTUP_BIN): $(OBJS) $(RUNTIME) $(CORE) $(THREADING) $(SERVERS_CORE) $(CLIENTS)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Servers.Startup)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-T $(STARTUP_LINKER) \
		$(OBJS) $(SERVERS_CORE) $(CLIENTS) $(RUNTIME) $(CORE) $(THREADING) -o $@.elf
	$(call ECHO,OBJCOPY $(REL)/$@,Quantum.Servers.Startup)
	@$(OBJCOPY32) -O binary $@.elf $@

clean:
	$(call ECHO,RM $(BUILD),Quantum.Servers.Startup)
	@rm -rf $(BUILD)
