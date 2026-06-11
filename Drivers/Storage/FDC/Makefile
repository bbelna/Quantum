# Makefile for IA32 FDC Driver

PROJECT_ROOT := $(abspath ../../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

FDC := $(BUILD)/FDC.qx

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean FDC

all: FDC

-include $(patsubst %.o,%.d,$(OBJS))

FDC: $(FDC)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Drivers.Storage.FDC)
	@$(CC32) $(CFLAGS32) -I$(PROJECT_INCLUDE) -I. -c $< -o $@

$(FDC): $(OBJS) $(RUNTIME) $(CORE) $(CLIENTS) $(SERVERS_CORE) $(THREADING)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Drivers.Storage.FDC)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(USER_LINKER) \
		$(OBJS) $(RUNTIME) $(CORE) $(CLIENTS) $(SERVERS_CORE) $(THREADING) -o $@

clean:
	$(call ECHO,RM $(BUILD),Quantum.Drivers.Storage.FDC)
	@rm -rf $(BUILD)
