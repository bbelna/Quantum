# Makefile for Quantum Threading library

PROJECT_ROOT ?= $(abspath ..)

-include $(PROJECT_ROOT)/Makefile.common.mk

INCLUDE = $(PROJECT_ROOT)/Include/Quantum/Threading

SRCS := $(wildcard *.cpp) $(wildcard OS/QuantumOS/*.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(SRCS))

.PHONY: all clean Quantum.Threading

all: Quantum.Threading

Quantum.Threading: $(THREADING)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Threading)
	@$(CC32) $(CFLAGS32) -I./ -I$(INCLUDE) -I$(PROJECT_INCLUDE) -c $< -o $@

$(THREADING): $(OBJS) $(CORE) $(CLIENTS) $(RUNTIME)
	@mkdir -p $(dir $@)
	$(call ECHO,AR $(REL)/$@,Quantum.Threading)
	@$(AR32) rcs $@ $(OBJS) \
		$(BUILD_ROOT)/Core/*.o \
		$(BUILD_ROOT)/Clients/*.o \
		$(BUILD_ROOT)/Runtime/OS/QuantumOS/*.o
	@$(RANLIB) $@

-include $(OBJS:.o=.d)

clean:
	$(call ECHO,RM $(BUILD),Quantum.Threading)
	@rm -rf $(BUILD)
