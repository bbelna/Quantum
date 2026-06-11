# Makefile for IA32 Run

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

RUN := $(BUILD)/RunServer.qbn
SRCS := \
	$(wildcard *.cpp) \
	$(wildcard Core/*.cpp) \
	$(wildcard Core/*/*.cpp) \
	$(wildcard Controllers/*.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean Run

all: Run

-include $(OBJS:.o=.d)

Run: $(RUN)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Servers.Run)
	@$(CC32) $(CFLAGS32) -I$(PROJECT_INCLUDE) \
		-I$(PROJECT_INCLUDE)/Quantum/Servers/Run -I. -I./Core \
		-c $< -o $@

$(RUN): $(OBJS) $(SERVERS_CORE) $(CLIENTS) $(CORE) $(THREADING) $(RUNTIME)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Servers.Run)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(USER_LINKER) \
		$(OBJS) $(SERVERS_CORE) $(CLIENTS) $(CORE) $(THREADING) $(RUNTIME) -o $@.elf
	$(call ECHO,OBJCOPY $(REL)/$@,Quantum.Servers.Run)
	@$(OBJCOPY32) -O binary $@.elf $@

clean:
	$(call ECHO,RM $(BUILD),Quantum.Servers.Run)
	@rm -rf $(BUILD)
