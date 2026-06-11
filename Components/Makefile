# Makefile for Quantum Components library

PROJECT_ROOT ?= $(abspath ..)

-include $(PROJECT_ROOT)/Makefile.common.mk

INCLUDE := $(PROJECT_ROOT)/Include/Quantum/Components

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(SRCS))

.PHONY: all clean Components

all: Components

Components: $(COMPONENTS)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Components)
	@$(CC32) $(CFLAGS32) -I. -I$(PROJECT_ROOT)/Include/ -c $< -o $@

$(COMPONENTS): $(OBJS)
	@mkdir -p $(dir $@)
	$(call ECHO,AR $(REL)/$@,Quantum.Components)
	@$(AR32) rcs $@ $(OBJS)
	@$(RANLIB) $@

-include $(OBJS:.o=.d)

clean:
	$(call ECHO,RM $(BUILD),Quantum.Components)
	@rm -rf $(BUILD)
