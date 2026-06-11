# Makefile for Quantum library

PROJECT_ROOT ?= $(abspath ..)

-include $(PROJECT_ROOT)/Makefile.common.mk

INCLUDE := $(PROJECT_ROOT)/Include

SRCS := $(wildcard Quantum*.cpp) $(wildcard OS/QuantumOS/*.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(SRCS))

.PHONY: all clean Runtime

all: Runtime

Runtime: $(RUNTIME)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Runtime)
	@$(CC32) $(CFLAGS32) -I$(INCLUDE) -I$(INCLUDE)/Quantum/Runtime -I$(INCLUDE)/Quantum/Runtime/OS/QuantumOS -c $< -o $@

$(RUNTIME): $(OBJS)
	@mkdir -p $(dir $@)
	$(call ECHO,AR $(REL)/$@,Quantum.Runtime)
	@$(AR32) rcs $@ $(OBJS)
	@$(RANLIB) $@

-include $(OBJS:.o=.d)

clean:
	$(call ECHO,RM $(BUILD),Quantum.Runtime)
	@rm -rf $(BUILD)
