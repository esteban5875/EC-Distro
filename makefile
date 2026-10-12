CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Isrc/include -MMD -MP
LDFLAGS  := -static

SHELL_SRCS := main.cpp $(shell find src/shell -name '*.cpp')
SHELL_OBJS := $(SHELL_SRCS:%.cpp=build/%.o)

TOOL_SRCS  := $(wildcard src/tools/*.cpp)
TOOL_BINS  := $(patsubst src/tools/%.cpp,bin/tools/%,$(TOOL_SRCS))
TOOL_OBJS  := $(TOOL_SRCS:%.cpp=build/%.o)

DEPS := $(SHELL_OBJS:.o=.d) $(TOOL_OBJS:.o=.d)

.PHONY: all clean container boot

all: bin/shell $(TOOL_BINS)
	mkdir -p bin/tools

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

clean:
	rm -rf bin build out

container:
	sudo docker build --target runtime -t mi-shell-lfs .
	sudo docker run -it --rm mi-shell-lfs

boot:
	sudo docker build --target artifacts --output type=local,dest=./out .
	qemu-system-x86_64 -kernel out/bzImage -initrd out/initramfs.cpio.gz \
	  -append "console=ttyS0 rdinit=/init" -nographic