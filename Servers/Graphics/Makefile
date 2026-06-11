# Makefile for IA32 Graphics

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

GRAPHICS := $(BUILD)/GraphicsServer.qx
SRCS := \
	$(wildcard *.cpp) \
	$(wildcard Core/*.cpp) \
	$(wildcard Core/*/*.cpp) \
	$(wildcard Controllers/*.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean Graphics

all: Graphics

-include $(OBJS:.o=.d)

Graphics: $(GRAPHICS)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Servers.Graphics)
	@$(CC32) $(CFLAGS32) -I$(PROJECT_INCLUDE) -I. -I./Core \
		-c $< -o $@

$(GRAPHICS): $(OBJS) $(RUNTIME) $(CORE) $(THREADING) $(SERVERS_CORE) $(CLIENTS)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Servers.Graphics)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(USER_LINKER) \
		$(OBJS) $(SERVERS_CORE) $(CLIENTS) $(RUNTIME) $(CORE) $(THREADING) -o $@

clean:
	$(call ECHO,RM $(BUILD),Quantum.Servers.Graphics)
	@rm -rf $(BUILD)
