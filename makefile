#Makefile -> Generated

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Isrc/include -MMD -MP
LDFLAGS  := -static

# ---------- Shell and base tools ----------
SHELL_SRCS := main.cpp $(shell find src/shell -name '*.cpp')
SHELL_OBJS := $(SHELL_SRCS:%.cpp=build/%.o)

TOOL_SRCS  := $(wildcard src/tools/*.cpp)
TOOL_BINS  := $(patsubst src/tools/%.cpp,bin/tools/%,$(TOOL_SRCS))
TOOL_OBJS  := $(TOOL_SRCS:%.cpp=build/%.o)

DEPS := $(SHELL_OBJS:.o=.d) $(TOOL_OBJS:.o=.d)

# ---------- Repo packages ----------
PKG_VERSION := 1.0
REPO_CXXFLAGS := -std=c++17 -Wall -Wextra

REPO_SRCS  := $(wildcard repo/source/*.cpp)
REPO_NAMES := $(basename $(notdir $(REPO_SRCS)))
REPO_TARS  := $(patsubst %,repo/packages/%-$(PKG_VERSION).tar,$(REPO_NAMES))

PKG_SRCS := $(wildcard src/pkg/*.cpp)
PKG_OBJS := $(PKG_SRCS:%.cpp=build/%.o)
DEPS     += $(PKG_OBJS:.o=.d)

.PHONY: all clean container boot repo
.SECONDARY:

all: bin/shell $(TOOL_BINS) bin/tools/pkg
	mkdir -p bin/tools

bin/tools/pkg: $(PKG_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) $^ -o $@
	
bin/shell: $(SHELL_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) $^ -o $@

# One binary per tool: src/tools/ls.cpp -> bin/tools/ls
$(TOOL_BINS): bin/tools/%: build/src/tools/%.o
	mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) $< -o $@

build/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

# ---------- Repo: repo/source/bye.cpp -> repo/packages/bye-1.0.tar + repo/index ----------
repo: repo/index

# 1. Compile each package as a static binary
repo/build/%: repo/source/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(REPO_CXXFLAGS) $(LDFLAGS) $< -o $@

# 2. Stage it as bin/<name> and pack it (paths inside the tar are relative to /)
repo/packages/%-$(PKG_VERSION).tar: repo/build/%
	rm -rf repo/stage/$*
	mkdir -p repo/stage/$*/bin $(dir $@)
	cp $< repo/stage/$*/bin/$*
	tar --owner=0 --group=0 --numeric-owner -cf $@ -C repo/stage/$* bin

# 3. Generate the index: name version filename
repo/index: $(REPO_TARS)
	mkdir -p repo
	for n in $(REPO_NAMES); do echo "$$n $(PKG_VERSION) $$n-$(PKG_VERSION).tar"; done > $@

clean:
	rm -rf bin build out repo/build repo/stage repo/packages repo/index

container: repo
	sudo docker build --target runtime -t mi-shell-lfs .
	sudo docker run -it --rm mi-shell-lfs

boot: repo
	sudo docker build --target artifacts --output type=local,dest=./out .
	qemu-system-x86_64 -m 256M -kernel out/bzImage -initrd out/initramfs.cpio.gz \
	  -append "console=ttyS0 rdinit=/init" -nographic