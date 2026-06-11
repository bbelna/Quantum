# Makefile for IA32 Storage Server

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

STORAGE := $(BUILD)/StorageServer.qx

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean Storage

all: Storage

-include $(patsubst %.o,%.d,$(OBJS))

Storage: $(STORAGE)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Servers.Storage)
	@$(CC32) $(CFLAGS32) -I$(PROJECT_INCLUDE) -I. -c $< -o $@

$(STORAGE): $(OBJS) $(SERVERS_CORE) $(CLIENTS) $(RUNTIME) $(CORE) $(THREADING)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Servers.Storage)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(USER_LINKER) \
		$(OBJS) $(SERVERS_CORE) $(CLIENTS) $(RUNTIME) $(CORE) $(THREADING) -o $@

clean:
	$(call ECHO,RM $(BUILD),Quantum.Servers.Storage)
	@rm -rf $(BUILD)
