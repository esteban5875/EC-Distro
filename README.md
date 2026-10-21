# EC-DistroV1

> Made with AI assistance. Build, release setup, README and orchestration was generated and reviewed with claude. Source code of the distro was hand-written and design choices made by me.

EC-DistroV1 is a small Linux distribution built from scratch to learn how the layers of a Linux system fit together. It has three parts: a shell written in C++17, a minimal kernel compiled from `tinyconfig`, and an initramfs where the shell runs as `/init`. The whole image boots in QEMU from a GRUB ISO, and the shell can also run on its own in a Docker container for quick testing.

This is an educational project, not production software.

## Requirements

- Linux host (x86_64)
- Python 3.8+
- Docker 23+ (BuildKit). Every build tool (compiler, kernel build deps, GRUB, mkfs) runs inside Docker.
- QEMU, only to boot: `qemu-system-x86_64`
  - Debian/Ubuntu: `sudo apt install qemu-system-x86`
  - Fedora: `sudo dnf install qemu-system-x86`
  - Arch: `sudo pacman -S qemu-full`

## Quick start

```sh
python3 ecdistro.py
```

This builds everything in Docker, asks for a virtual disk size (for example `512M` or `2G`), and boots the ISO in QEMU. The first build downloads and compiles the kernel, which takes a few minutes. Later builds reuse the Docker cache.

Inside the guest, type `exit` to power off. `Ctrl-A` then `X` kills QEMU.

`make boot` does the same thing.

## The virtual disk

Each `run` creates a fresh qcow2 disk of the size you choose, formatted as ext2. The guest's init mounts it at `/data`. **The disk is deleted when QEMU exits**, so nothing written to `/data` (or anywhere else) survives between runs. The disk is scratch space for experimenting with a real block device and filesystem.

## Commands

| Command | What it does |
|---|---|
| `python3 ecdistro.py` / `all` | `build`, then `run` |
| `build` | Builds `out/ec-distro-<version>.iso`, `out/bzImage`, `out/initramfs.cpio.gz` and the disk tools image |
| `run` | Creates the virtual disk, boots the ISO, deletes the disk on exit |
| `test` | Headless boot check: boots, checks `/data`, runs a command, powers off |

Useful options (see `python3 ecdistro.py <command> --help`):

- `--disk-size 2G`: skip the size prompt (required when stdin is not a terminal)
- `--memory 512M`: guest RAM (default `256M`)
- `--sudo`: run Docker through `sudo` if your user isn't in the `docker` group
- `--kernel-version 6.12.5`: build another kernel version (its download checksum is not verified)
- `--docker-arg=--network=host`: pass extra flags to `docker build`, for example behind a proxy
- `--no-kvm`: don't use KVM even when `/dev/kvm` is available

## Prebuilt ISO

Each tagged release on GitHub has the ISO, kernel and initramfs attached, plus `SHA256SUMS`. To boot a downloaded ISO you still need the disk tools image once (`python3 ecdistro.py build`), or boot it without a disk:

```sh
qemu-system-x86_64 -m 256M -cdrom ec-distro-1.0.0.iso -boot d -nographic -no-reboot
```

## Limitations

- The ISO boots with legacy BIOS only (QEMU's default). UEFI boot is not supported yet.
- No package or file state persists between runs.

## Project layout

- `main.cpp`, `src/shell/`: the shell, which is also PID 1 (`/init`)
- `src/tools/`: small standalone tools (`ls`, `hello`), one binary each
- `src/pkg/`: `pkg`, the package manager; packages come from `repo/source/`
- `boot/grub.cfg`: GRUB menu for the ISO
- `Dockerfile`: build stages for the shell, kernel, initramfs, ISO and disk tools
- `ecdistro.py`: build and boot orchestration
