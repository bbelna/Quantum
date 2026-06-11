# Makefile for Quantum OS Clients library

PROJECT_ROOT := $(abspath ..)

-include $(PROJECT_ROOT)/Makefile.common.mk

INCLUDE := $(PROJECT_ROOT)/Include/Quantum/Clients

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(SRCS))

.PHONY: all clean Clients

all: Clients

Clients: $(CLIENTS)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Clients)
	@$(CC32) $(CFLAGS32) -I. -I$(INCLUDE) -I$(PROJECT_INCLUDE) -c $< -o $@

$(CLIENTS): $(OBJS)
	@mkdir -p $(dir $@)
	$(call ECHO,AR $(REL)/$@,Quantum.Clients)
	@$(AR32) rcs $@ $(OBJS)
	@$(RANLIB) $@

-include $(OBJS:.o=.d)

clean:
	$(call ECHO,RM $(BUILD),Quantum.Clients)
	@rm -rf $(BUILD)
