# Makefile for FAT12 Filesystem Server

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

FAT12 := $(BUILD)/FAT12.qx

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean FAT12

all: FAT12

-include $(patsubst %.o,%.d,$(OBJS))

FAT12: $(FAT12)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.FileSystems.FAT12)
	@$(CC32) $(CFLAGS32) -I$(PROJECT_INCLUDE) -c $< -o $@

$(FAT12): $(OBJS) $(RUNTIME) $(CORE) $(THREADING)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.FileSystems.FAT12)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(USER_LINKER) \
		$(OBJS) $(RUNTIME) $(CORE) $(THREADING) -o $@

clean:
	$(call ECHO,RM $(BUILD),Quantum.FileSystems.FAT12)
	@rm -rf $(BUILD)
