CXX      := g++
CXXFLAGS := -Wall -Wextra -O2 -std=c++17

SRC_DIR  := source
BIN_DIR  := bin

PROGS_DIRS := $(wildcard $(SRC_DIR)/*)
PROGS      := $(notdir $(PROGS_DIRS))

TARGETS    := $(foreach prog,$(PROGS),$(BIN_DIR)/$(prog)/$(prog))

all: $(TARGETS)

define PROGRAM_TEMPLATE
# Encuentra los archivos .cpp de este programa específico
SRCS_$(1) := $$(shell find $$(SRC_DIR)/$(1) -type f -name "*.cpp")

OBJS_$(1) := $$(patsubst $$(SRC_DIR)/$(1)/%,$$(BIN_DIR)/$(1)/obj/%,$$(SRCS_$(1):.cpp=.o))

$$(BIN_DIR)/$(1)/obj/%.o: $$(SRC_DIR)/$(1)/%.cpp
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(CXXFLAGS) -c $$< -o $$@

$$(BIN_DIR)/$(1)/$(1): $$(OBJS_$(1))
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(CXXFLAGS) $$^ -o $$@
endef

$(foreach prog,$(PROGS),$(eval $(call PROGRAM_TEMPLATE,$(prog))))

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean
