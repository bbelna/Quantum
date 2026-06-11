# Makefile for IA32 ATA/IDE Driver

PROJECT_ROOT := $(abspath ../../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

ATA := $(BUILD)/ATA.qx

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean ATA

all: ATA

-include $(patsubst %.o,%.d,$(OBJS))

ATA: $(ATA)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Drivers.Storage.ATA)
	@$(CC32) $(CFLAGS32) -I$(PROJECT_INCLUDE) -I. -c $< -o $@

$(ATA): $(OBJS) $(RUNTIME) $(CORE) $(CLIENTS) $(SERVERS_CORE) $(THREADING)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Drivers.Storage.ATA)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(USER_LINKER) \
		$(OBJS) $(RUNTIME) $(CORE) $(CLIENTS) $(SERVERS_CORE) $(THREADING) -o $@

clean:
	$(call ECHO,RM $(BUILD),Quantum.Drivers.Storage.ATA)
	@rm -rf $(BUILD)
