# EC-DistroV1

EC-DistroV1 is a small Linux distribution built from scratch to learn how the layers of a Linux system fit together. It has three parts: a shell written in C++17, a minimal kernel compiled from `tinyconfig`, and an initramfs where the shell runs as `/init`. The whole image boots in QEMU, and the shell can also run on its own in a Docker container for quick testing.

