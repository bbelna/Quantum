# Makefile for IA32 ContextServer

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

CONTEXT := $(BUILD)/Content.qx
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean Context

all: Context

-include $(OBJS:.o=.d)

Context: $(CONTEXT)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Servers.Context)
	@$(CC32) $(CFLAGS32) -I. -I$(PROJECT_INCLUDE) -c $< -o $@

$(CONTEXT): $(OBJS) $(SERVERS_CORE) $(CLIENTS) $(CORE) $(THREADING) $(COMPONENTS) $(MENUS) $(UI) $(FONTS) $(RUNTIME)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Servers.Context)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I. -I$(PROJECT_INCLUDE) \
		-T $(SERVER_LINKER) \
		$(OBJS) $(SERVERS_CORE) $(CLIENTS) $(COMPONENTS) $(MENUS) $(UI) $(FONTS) $(CORE) $(THREADING) $(RUNTIME) -o $@

clean:
	$(call ECHO,RM $(BUILD),Quantum.Servers.Context)
	@rm -rf $(BUILD)
