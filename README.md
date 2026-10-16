# EC-DistroV1

EC-DistroV1 is a small Linux distribution built from scratch to learn how the layers of a Linux system fit together. It has three parts: a shell written in C++17, a minimal kernel compiled from `tinyconfig`, and an initramfs where the shell runs as `/init`. The whole image boots in QEMU, and the shell can also run on its own in a Docker container for quick testing.

The project is in early development. The shell has a few built-ins and can run external programs, a few standalone tools live in `/bin`, and `pkg` can install extra programs from a local repository. The shell does not yet do the work a real PID 1 has to do, so there is no persistent disk and the package repository is a temporary setup (see [Package manager](#package-manager-pkg)).

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

`src/tools/` holds small standalone programs, one binary per `.cpp` file, installed to `/bin` in the image. It currently includes `ls` and `hello` (a simple test command). More tools will be added over time, so check `src/tools/` for the current list.

`repo/` holds extra programs that are not built into `/bin`. They are packaged as tarballs and installed at runtime with `pkg` (see below).

### Output: print buffer vs `std::cout`

The shell and the tools in `src/tools/` don't write to `std::cout` directly. They format each line with `printLine()` / `errLine()`, which `vsnprintf` into a fixed 4096-byte buffer (`SHELL_PRINT_BUFFER` in `src/header/constants.h`, `CHILD_SHELL_PRINT_BUFFER` in `src/tools/common/constants.h`) and then write that buffer to `std::cout` or `std::cerr`. This is an educational choice, to work with a fixed buffer and formatted output by hand. It is not needed for performance, and a line longer than the buffer is rejected with an error.

Programs in `repo/` are the exception: they must use plain `std::cout` and do not use the print buffer helpers.

### Package manager (`pkg`)

`pkg` is built from `src/pkg/` and installed to `/bin/pkg` like the other tools. It installs packages from a local repository into a root directory and keeps a record of every file it wrote, so a package can be removed cleanly.

```
pkg [--root DIR] [--repo DIR] <list|info|install|remove> [name]
```

| Command | Description |
|---------|-------------|
| `pkg list` | List every package in the repository index, marking the installed ones with `[installed]` |
| `pkg info <name>` | Show the available version and, if installed, the installed version and its files |
| `pkg install <name>` | Extract the package into the root and record its files |
| `pkg remove <name>` | Delete the recorded files, then any directories the install created that are now empty |

`--root` defaults to `/` and `--repo` defaults to `/mnt/repo`.

How it works:

- **Repository.** `<repo>/index` has one line per package: `name version filename [sha256]`. The tarballs live in `<repo>/packages/`. The sha256 field is parsed but not checked yet.
- **Packages.** Each package is a plain ustar archive whose paths are relative to `/`, for example `bin/bye`. `pkg` has its own tar reader; it accepts only regular files and directories, and rejects absolute paths and `..`.
- **Installed database.** For each installed package, `<root>/var/lib/pkg/<name>` stores the version, the directories it created (`d`) and the files it wrote (`f`).
- **Safety.** Files are written to a temporary name and renamed into place, with the mode bits from the archive. `pkg` refuses to overwrite a file that already exists, and if an install fails partway it removes what it had written.

#### Temporary setup: no PID 1 yet

The shell does not mount anything at boot yet, so there is no separate disk for the repository and no persistent root. Until that exists, the repository is copied straight into the image (the lines marked `TEMPORARY` in the Dockerfile):

| Environment | Repository location | Notes |
|-------------|---------------------|-------|
| QEMU (`make boot`) | `/mnt/repo`, inside the initramfs | Installed packages live in RAM and are lost on reboot. |
| Docker (`make container`) | `/mnt/repo`, inside the image | Installed packages are lost when the container exits. |

Both match the default `--repo`, so a plain `pkg list` works in either.

This will change once PID 1 duties and a persistent disk land: the repository moves to its own disk and the copy into the image goes away.

#### Adding a package

1. Create `repo/source/<name>.cpp` with its own `main()`, using plain `std::cout`.
2. Run `make repo`. It compiles a static binary into `repo/build/`, stages it as `bin/<name>` in `repo/stage/<name>/`, packs `repo/packages/<name>-1.0.tar`, and regenerates `repo/index`.

All packages currently share one version, `PKG_VERSION` in the makefile.

### Kernel and image

The Dockerfile downloads Linux 6.12.1 by default and builds it from `tinyconfig` with the options needed to boot from an initramfs and talk over a serial console. The shell becomes `/init`, the tools and `pkg` go to `/bin`, and the package repository is copied to `/mnt/repo` for now. `make boot` exports `bzImage` and `initramfs.cpio.gz` and starts them in QEMU.

## Project layout

```
main.cpp              Shell entry point: prompt loop and command dispatch
src/include.h         Prompt and clear-screen helpers
src/header/           Shared headers (command types, built-in declarations, constants)
src/shell/            Built-ins (echo, type, cd, pwd), parsing, PATH lookup, process launching
src/tools/            Standalone tools, one binary per .cpp file
src/pkg/              Package manager: CLI, index, tar extraction, installed database
repo/source/          Package sources, one package per .cpp file
repo/build/  repo/stage/  repo/packages/  repo/index
                      Generated by `make repo`
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
| `make` (or `make all`) | Build `bin/shell`, every tool and `pkg` into `bin/tools/` |
| `make repo` | Build every package in `repo/source/` and regenerate `repo/index` |
| `make clean` | Remove `bin/`, `build/`, `out/` and everything `make repo` generated |
| `make container` | Run `make repo`, build the Docker `runtime` image, and run the shell in it |
| `make boot` | Run `make repo`, build the kernel and initramfs through Docker, export them to `out/`, and boot them in QEMU |

### How the makefile works

Everything is compiled with `g++ -std=c++17 -Wall -Wextra` and linked with `-static`. The image has no libc and no dynamic loader, so a dynamically linked binary would fail there with a misleading "no such file or directory".

The shell is built from `main.cpp` plus every `.cpp` under `src/shell/`, which `find` picks up automatically. Each file in `src/tools/` becomes its own binary, so `src/tools/ls.cpp` turns into `bin/tools/ls`. All `.cpp` files in `src/pkg/` are linked together into `bin/tools/pkg`. Object files go to `build/`, and `-MMD -MP` tracks header dependencies so that editing a header rebuilds only the files that include it. Adding a new source file never requires editing the makefile.

### Docker stages

1. `shell-builder` builds the shell and tools on Alpine.
2. `kernel-builder` downloads the kernel (`KERNEL_VERSION`, default 6.12.1) and builds a `tinyconfig` kernel with initrd, serial console, devtmpfs, procfs and sysfs enabled.
3. `initramfs-builder` installs the shell as `/init` and the tools in `/bin`, copies the package repository to `/mnt/repo` (temporary), then packs everything into `initramfs.cpio.gz`.
4. `artifacts` exports `bzImage` and `initramfs.cpio.gz` (used by `make boot`).
5. `runtime` is a minimal image with the shell, the tools and a copy of the repository at `/mnt/repo` (used by `make container`).

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
- [x] `pkg`, a package manager using ustar tarballs, an index and a local repository (repository baked into the image for now)

Next:

- [ ] More tools in `src/tools/` (`cat`, `mkdir` and similar; `ls` is done)
- [ ] Better parsing: quotes, escape sequences in `echo`, `$VAR` expansion
- [ ] `cd` with no argument going to `HOME`, and `cd -`
- [ ] PID 1 duties: mount `/proc`, `/sys` and `/dev`, reap orphaned processes, and power off on `exit` instead of returning from `/init`
- [ ] Pipes and output redirection

Later:

- [ ] Persistent ext4 disk and a real bootloader
- [ ] Move the package repository to its own disk and drop the copy in the image
- [ ] `pkg`: verify sha256 checksums, per-package versions, upgrades
- [ ] Networking and an HTTP package repository (V2)
