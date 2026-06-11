# Makefile for IA32 FileSystem

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

FILESYSTEM := $(BUILD)/FileSystemServer.qx
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean FileSystem

all: FileSystem

-include $(OBJS:.o=.d)

FileSystem: $(FILESYSTEM)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Servers.FileSystem)
	@$(CC32) $(CFLAGS32) -I. -I$(PROJECT_INCLUDE) -c $< -o $@

$(FILESYSTEM): $(OBJS) $(RUNTIME) $(CORE) $(THREADING) $(SERVERS_CORE) $(CLIENTS)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Servers.FileSystem)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(USER_LINKER) \
		$(OBJS) $(SERVERS_CORE) $(CLIENTS) $(RUNTIME) $(CORE) $(THREADING) -o $@

clean:
	$(call ECHO,RM $(BUILD),Quantum.Servers.FileSystem)
	@rm -rf $(BUILD)
