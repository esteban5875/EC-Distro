# Builds the EC-Distro userland (../Userlandsource).
#
#   make              build everything into build/rootfs/bin
#   make shell        only the shell (main.cpp + core/shell)
#   make tools        hello, ls
#   make pkg          package manager
#   make clean
#
# Override as needed: make CXX=/opt/gcc/bin/g++ STATIC=0 JOBS=...

SRC      := ../Userlandsource
BUILD    := build
OBJ      := $(BUILD)/obj
BIN      := $(BUILD)/rootfs/bin

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
STATIC   ?= 1
ifeq ($(STATIC),1)
LDFLAGS  += -static
endif

CPPFLAGS += -I$(SRC)

SHELL_SRCS := $(SRC)/main.cpp $(wildcard $(SRC)/core/shell/*.cpp)
PKG_SRCS   := $(wildcard $(SRC)/core/pkg/*.cpp)
TOOLS      := hello ls

SHELL_OBJS := $(patsubst $(SRC)/%.cpp,$(OBJ)/%.o,$(SHELL_SRCS))
PKG_OBJS   := $(patsubst $(SRC)/%.cpp,$(OBJ)/%.o,$(PKG_SRCS))

.SECONDARY:
.PHONY: all shell tools pkg clean
all: shell tools pkg

shell: $(BIN)/ecsh
tools: $(addprefix $(BIN)/,$(TOOLS))
pkg:   $(BIN)/pkg

$(BIN)/ecsh: $(SHELL_OBJS) | $(BIN)
	$(CXX) $(LDFLAGS) $^ -o $@

$(BIN)/pkg: $(PKG_OBJS) | $(BIN)
	$(CXX) $(LDFLAGS) $^ -o $@

# Each tool is a single file: core/tools/<name>.cpp
$(BIN)/%: $(OBJ)/core/tools/%.o | $(BIN)
	$(CXX) $(LDFLAGS) $< -o $@

$(OBJ)/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BIN):
	@mkdir -p $@

clean:
	rm -rf $(BUILD)

-include $(shell find $(OBJ) -name '*.d' 2>/dev/null)
