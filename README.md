# EC-DistroV1

A tiny Linux "distro" built from scratch: a minimal Linux kernel plus a custom shell written in C++17, packaged into an initramfs that can boot in QEMU (or a container for quick testing).

> **Status:** early work in progress. The shell has a handful of built-ins; external command execution and the `src/tools/` utilities are not implemented yet.

## Objective

The goal is to understand how a Linux system comes together by building every layer myself:

1. **A custom shell** (`bin/shell`), statically linked, which runs as PID 1 (`/init`) inside the initramfs.
2. **Small standalone tools** (one binary per file in `src/tools/`, e.g. `ls`), installed to `/bin` in the image.
3. **A minimal kernel** compiled from `tinyconfig` with only the options needed to boot and talk over a serial console.
4. **A boot image** (`bzImage` + `initramfs.cpio.gz`) runnable in QEMU, a VM or real hardware.

## Current features

Built-in shell commands:

| Command | Description |
|---------|-------------|
| `echo`  | Print arguments |
| `type`  | Show whether a command is a built-in or where it is found in `PATH` |
| `pwd`   | Print the working directory |
| `cd`    | Change the working directory |
| `clear` | Clear the screen |
| `exit`  | Leave the shell |

Not yet implemented: running external programs (the shell prints `Unknown command:` for them) and the `src/tools/` utilities.

## Project layout

```
main.cpp              Shell entry point: prompt loop and command dispatch
src/include.h         Prompt, clear screen, and command exposure
src/header/           Shared headers (commands, constants, shell types)
src/shell/            Built-in implementations (echo, type, cd, pwd) and parsing
src/tools/            (planned) standalone tools, one binary per .cpp file
makefile              Build rules
Dockerfile            Multi-stage build: shell, kernel, initramfs, boot artifacts
bin/  build/  out/    Generated output (git-ignored)
```

## Makefile tutorial

The [makefile](makefile) compiles with `g++ -std=c++17 -Wall -Wextra` and links **statically** (`-static`), so the binaries have no runtime dependencies and can live in an empty initramfs.

### How it works

- `SHELL_SRCS` = `main.cpp` + every `.cpp` under `src/shell/` (found automatically with `find`).
- Each `.cpp` is compiled to an object file in `build/` (`build/%.o: %.cpp`).
- Objects are linked into `bin/shell`.
- `src/tools/*.cpp` is picked up with a wildcard. Each file becomes its own binary: `src/tools/ls.cpp` → `bin/tools/ls`.
- `-MMD -MP` generates `.d` dependency files, so editing a header rebuilds only what depends on it.

Adding a new built-in or tool needs no makefile edits: drop the `.cpp` into `src/shell/` or `src/tools/`.

### Targets

| Command | What it does |
|---------|--------------|
| `make` (or `make all`) | Build `bin/shell` and all tools in `bin/tools/` |
| `make clean` | Remove `bin/`, `build/` and `out/` |
| `make container` | Build the Docker `runtime` image and run the shell in it (interactive, removed on exit) |
| `make boot` | Build the kernel and initramfs through Docker, export them to `out/`, and boot them in QEMU |

### Quick start

Build and run the shell directly on your machine:

```bash
make
./bin/shell
```

Try it in a container (needs Docker):

```bash
make container
```

Boot the full image in QEMU (needs Docker and `qemu-system-x86_64`; the first run compiles a kernel, which takes a while):

```bash
make boot
```

To exit QEMU in `-nographic` mode press `Ctrl+A`, then `X`.

### Requirements

- `g++` with C++17 support and static libc (`glibc-static` on some distros), `make`, `find`
- Docker (for `make container` and `make boot`)
- `qemu-system-x86_64` (for `make boot`)
- `sudo` access, since the Docker targets call `sudo docker`

## Docker build stages

The [Dockerfile](Dockerfile) is split into stages:

1. `shell-builder`: builds the shell and tools on Alpine.
2. `kernel-builder`: downloads Linux (`KERNEL_VERSION`, default 6.12.1) and builds a `tinyconfig` kernel with serial console, initrd and devtmpfs enabled.
3. `initramfs-builder`: places the shell as `/init` and tools in `/bin`, then packs everything into `initramfs.cpio.gz`.
4. `artifacts`: exports `bzImage` and `initramfs.cpio.gz` (used by `make boot`).
5. `runtime`: minimal image holding just the shell (used by `make container`).


## Design notes

- **Static linking is mandatory.** The image has no libc or dynamic loader, so a dynamic binary fails with a misleading "no such file or directory".
- **Lookup order:** builtin, then `PATH` (or the exact path if the name contains `/`), then "not found".
- **Exit codes:** 126 for found-but-not-runnable, 127 for not found.
- **`PATH` defaults to `/bin`** since a kernel-started `/init` has almost no environment.
- **Docker is only the build environment.** The distro itself runs on its own kernel in QEMU.

### Adding a tool

1. Create `src/tools/<name>.cpp` with its own `main()`.
2. Run `make`. The binary appears at `bin/tools/<name>` and ends up in `/bin` inside the distro.

### Adding a builtin

1. Add a value to `CommandName` and an entry to `CommandMap` (this also makes `type` report it).
2. Write the function in `src/shell/`.
3. Add its case to the dispatch `switch` in `main.cpp`.

## Roadmap

**Done**
- [x] Static C++ shell with a REPL loop
- [x] Builtins: `echo`, `type`, `pwd`, `cd`
- [x] PATH lookup with explicit result statuses (found / not found / not executable / not a regular file / PATH unset)
- [x] Reproducible Docker build: static userland, minimal kernel (`tinyconfig`), initramfs

**In progress / planned**
- [ ] Run external commands (`fork` + `execv`)
- [ ] PID 1 setup: mount `/proc`, `/sys`, `/dev`; `exit` powers off instead of returning
- [ ] Persistent ext4 disk and a real bootloader
- [ ] `pkg` package manager (tarball + manifest, local repo)
- [ ] Networking and HTTP repo (V2)