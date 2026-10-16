# EC-DistroV1

EC-DistroV1 is a small Linux distribution built from scratch to learn how the layers of a Linux system fit together. It has three parts: a shell written in C++17, a minimal kernel compiled from `tinyconfig`, and an initramfs where the shell runs as `/init`. The whole image boots in QEMU, and the shell can also run on its own in a Docker container for quick testing.

The project is in early development. The shell has a few built-ins and can run external programs, but it does not yet do the work a real PID 1 has to do, and there are no standalone tools in `/bin` yet.

## Current state

### Shell

The shell reads a line, splits it on whitespace, and either runs a built-in or looks the command up in `PATH` and runs it with `fork` and `execv`. The prompt shows the current working directory.

| Command | Description |
|---------|-------------|
| `echo`  | Print its arguments separated by spaces |
| `type`  | Report whether a name is a built-in or where it is found in `PATH` |
| `pwd`   | Print the working directory |
| `cd`    | Change the working directory (one argument; `cd` with no argument does nothing yet) |
| `clear` | Clear the screen and scrollback |
| `exit`  | Print `Bye` and leave the shell |

Anything else is treated as an external command. If the name contains a `/`, the shell uses it as a path; otherwise it searches each directory in `PATH`. `PATH` defaults to `/bin` when it is not set, because a kernel-started `/init` gets almost no environment.

Failures use the usual shell exit codes: 127 when the command is not found, 126 when a file is found but is not executable or not a regular file. A program killed by a signal reports 128 plus the signal number.

There is no quoting, escaping, piping, redirection, or variable expansion yet.

### Tools

`src/tools/` is meant to hold small standalone programs, one binary per `.cpp` file, installed to `/bin` in the image. Right now it only contains `hello.cpp`, a simple test command.

### Kernel and image

The Dockerfile downloads Linux 6.12.1 by default and builds it from `tinyconfig` with the options needed to boot from an initramfs and talk over a serial console. The shell becomes `/init` and the tools go to `/bin`. `make boot` exports `bzImage` and `initramfs.cpio.gz` and starts them in QEMU.

## Project layout

```
main.cpp              Shell entry point: prompt loop and command dispatch
src/include.h         Prompt and clear-screen helpers
src/header/           Shared headers (command types, built-in declarations, constants)
src/shell/            Built-ins (echo, type, cd, pwd), parsing, PATH lookup, process launching
src/tools/            Standalone tools, one binary per .cpp file
makefile              Build rules
Dockerfile            Multi-stage build: shell, kernel, initramfs, boot artifacts
bin/  build/  out/    Generated output (git-ignored)
```

## Building and running

### Requirements

- `g++` with C++17 support and a static libc (`glibc-static` on some distros), plus `make` and `find`
- Docker, for `make container` and `make boot`
- `qemu-system-x86_64`, for `make boot`
- `sudo`, since the Docker targets call `sudo docker`

### Quick start

Build and run the shell on your machine:

```bash
make
./bin/shell
```

Run it in a container:

```bash
make container
```

Boot the full image in QEMU. The first run compiles a kernel, so it takes a while:

```bash
make boot
```

To leave QEMU in `-nographic` mode, press `Ctrl+A`, then `X`.

### Make targets

| Command | What it does |
|---------|--------------|
| `make` (or `make all`) | Build `bin/shell` and every tool into `bin/tools/` |
| `make clean` | Remove `bin/`, `build/` and `out/` |
| `make container` | Build the Docker `runtime` image and run the shell in it |
| `make boot` | Build the kernel and initramfs through Docker, export them to `out/`, and boot them in QEMU |

### How the makefile works

Everything is compiled with `g++ -std=c++17 -Wall -Wextra` and linked with `-static`. The image has no libc and no dynamic loader, so a dynamically linked binary would fail there with a misleading "no such file or directory".

The shell is built from `main.cpp` plus every `.cpp` under `src/shell/`, which `find` picks up automatically. Each file in `src/tools/` becomes its own binary, so `src/tools/ls.cpp` turns into `bin/tools/ls`. Object files go to `build/`, and `-MMD -MP` tracks header dependencies so that editing a header rebuilds only the files that include it. Adding a new source file never requires editing the makefile.

### Docker stages

1. `shell-builder` builds the shell and tools on Alpine.
2. `kernel-builder` downloads the kernel (`KERNEL_VERSION`, default 6.12.1) and builds a `tinyconfig` kernel with initrd, serial console, devtmpfs, procfs and sysfs enabled.
3. `initramfs-builder` installs the shell as `/init` and the tools in `/bin`, then packs everything into `initramfs.cpio.gz`.
4. `artifacts` exports `bzImage` and `initramfs.cpio.gz` (used by `make boot`).
5. `runtime` is a minimal image with just the shell (used by `make container`).

Docker is only the build environment. The distro itself runs on its own kernel in QEMU.

## Extending the shell

### Adding a built-in

1. Add a value to `CommandName` and an entry to `CommandMap` in `src/header/shell.h`. This also makes `type` report it as a built-in.
2. Declare the function in `src/header/builtInCommands.h` and write it in `src/shell/`.
3. Add a case for it in `parse()` in `src/shell/shell.cpp` and in the dispatch `switch` in `main.cpp`.

### Adding a tool

1. Create `src/tools/<name>.cpp` with its own `main()`.
2. Run `make`. The binary appears at `bin/tools/<name>` and is installed to `/bin` inside the image.

## Roadmap

Done:

- [x] Static C++ shell with a read-eval loop and a prompt showing the working directory
- [x] Built-ins: `echo`, `type`, `pwd`, `cd`, `clear`, `exit`
- [x] `PATH` lookup that distinguishes found, not found, not executable, not a regular file, and `PATH` unset
- [x] Running external commands with `fork` and `execv`, with 126/127 exit codes
- [x] Docker build of a static userland, a `tinyconfig` kernel and an initramfs, bootable in QEMU

Next:

- [ ] First real tools in `src/tools/` (`ls`, `cat`, `mkdir` and similar)
- [ ] Better parsing: quotes, escape sequences in `echo`, `$VAR` expansion
- [ ] `cd` with no argument going to `HOME`, and `cd -`
- [ ] PID 1 duties: mount `/proc`, `/sys` and `/dev`, reap orphaned processes, and power off on `exit` instead of returning from `/init`
- [ ] Pipes and output redirection

Later:

- [ ] Persistent ext4 disk and a real bootloader
- [ ] `pkg`, a package manager using tarballs with a manifest and a local repository
- [ ] Networking and an HTTP package repository (V2)
