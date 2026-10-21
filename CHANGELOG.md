# Changelog

## 1.0.0

First release.

- `ecdistro.py`: one script that builds everything in Docker, asks for a virtual disk size, and boots the result in QEMU. Commands: `build`, `run`, `test`, `all` (default).
- Bootable GRUB ISO (`out/ec-distro-<version>.iso`). Legacy BIOS only.
- Throwaway virtual disk: a fresh ext2 qcow2 disk is created for each run, mounted by init at `/data`, and deleted when QEMU exits.
- Kernel: virtio block and ext2 support added to the `tinyconfig` build. The build fails if a required option is dropped. The kernel tarball's SHA256 is pinned.
- Repo packages are built inside Docker, so the host no longer needs a compiler.
- CI builds and boot-tests every push and pull request. Tags `v*` publish a GitHub Release with the ISO, kernel, initramfs and `SHA256SUMS`.
