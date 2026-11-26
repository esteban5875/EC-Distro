# Builds the EC-Distro userland (../Userlandsource).
#
#   make              build everything into build/rootfs/bin
#   make shell        only the shell (main.c + core/shell)
#   make tools        hello, ls
#   make pkg          package manager
#   make clean
#
# Override as needed: make CC=/opt/gcc/bin/gcc STATIC=0 JOBS=...

SRC      := ../Userlandsource
BUILD    := build
OBJ      := $(BUILD)/obj
BIN      := $(BUILD)/rootfs/bin

CC       ?= gcc
CFLAGS   ?= -std=gnu11 -O2 -Wall -Wextra
STATIC   ?= 1
ifeq ($(STATIC),1)
LDFLAGS  += -static
endif

CPPFLAGS += -I$(SRC)

SHELL_SRCS := $(SRC)/main.c $(wildcard $(SRC)/core/shell/*.c)
PKG_SRCS   := $(wildcard $(SRC)/core/pkg/*.c)
TOOLS      := hello ls

SHELL_OBJS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SHELL_SRCS))
PKG_OBJS   := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(PKG_SRCS))

.SECONDARY:
.PHONY: all shell tools pkg clean
all: shell tools pkg

shell: $(BIN)/ecsh
tools: $(addprefix $(BIN)/,$(TOOLS))
pkg:   $(BIN)/pkg

$(BIN)/ecsh: $(SHELL_OBJS) | $(BIN)
	$(CC) $(LDFLAGS) $^ -o $@

$(BIN)/pkg: $(PKG_OBJS) | $(BIN)
	$(CC) $(LDFLAGS) $^ -o $@

# Each tool is a single file: core/tools/<name>.c
$(BIN)/%: $(OBJ)/core/tools/%.o | $(BIN)
	$(CC) $(LDFLAGS) $< -o $@

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BIN):
	@mkdir -p $@

clean:
	rm -rf $(BUILD)

-include $(shell find $(OBJ) -name '*.d' 2>/dev/null)
