# Builds EC-Distro inside Docker: the shell, tools and packages, a tinyconfig
# kernel, the initramfs and a bootable GRUB ISO (BIOS boot).
# Normally driven by ecdistro.py; see README.md.
#
#   artifacts  -> bzImage, initramfs.cpio.gz, ec-distro.iso  (docker build --output)
#   tools      -> image with mkfs.ext2 + qemu-img, used to create the virtual disk

# ---------- Stage 1: shell, tools, pkg and repo packages ----------
FROM alpine:3.20 AS shell-builder

# GNU tar: the makefile's repo rule uses --owner/--group/--numeric-owner.
RUN apk add --no-cache build-base make findutils tar

WORKDIR /build
COPY src/ ./src/
COPY repo/source/ ./repo/source/
COPY main.cpp makefile ./
RUN make all repo

# ---------- Stage 2: kernel ----------
FROM alpine:3.20 AS kernel-builder

RUN apk add --no-cache \
    build-base linux-headers bc flex bison perl bash \
    openssl-dev elfutils-dev curl xz diffutils findutils

# KERNEL_SHA256 must match KERNEL_VERSION. An empty value skips the check
# (ecdistro.py does this when --kernel-version is given).
ARG KERNEL_VERSION=6.12.1
ARG KERNEL_SHA256=0193b1d86dd372ec891bae799f6da20deef16fc199f30080a4ea9de8cef0c619
WORKDIR /kernel
RUN curl -fsSLo /tmp/linux.tar.xz \
      "https://cdn.kernel.org/pub/linux/kernel/v${KERNEL_VERSION%%.*}.x/linux-${KERNEL_VERSION}.tar.xz" && \
    if [ -n "${KERNEL_SHA256}" ]; then \
      echo "${KERNEL_SHA256}  /tmp/linux.tar.xz" | sha256sum -c -; \
    fi && \
    tar -xJf /tmp/linux.tar.xz --strip-components=1 && \
    rm /tmp/linux.tar.xz

# BLOCK..EXT2_FS: virtio disk support, so init can mount the virtual disk at /data.
ARG KERNEL_OPTS="64BIT BLK_DEV_INITRD RD_GZIP BINFMT_ELF BINFMT_SCRIPT PRINTK TTY \
SERIAL_8250 SERIAL_8250_CONSOLE DEVTMPFS DEVTMPFS_MOUNT PROC_FS SYSFS FUTEX \
BLOCK PCI VIRTIO_MENU VIRTIO VIRTIO_PCI BLK_DEV VIRTIO_BLK EXT2_FS"

RUN make tinyconfig && \
    for opt in ${KERNEL_OPTS}; do scripts/config -e "$opt"; done && \
    make olddefconfig && \
    for opt in ${KERNEL_OPTS}; do \
      grep -qx "CONFIG_${opt}=y" .config || { echo "kernel config: CONFIG_${opt} was dropped" >&2; exit 1; }; \
    done && \
    make -j"$(nproc)" bzImage

# ---------- Stage 3: initramfs ----------
FROM alpine:3.20 AS initramfs-builder

RUN apk add --no-cache cpio gzip

COPY --from=shell-builder /build/bin/shell /initramfs/init
COPY --from=shell-builder /build/bin/tools/ /initramfs/bin/

COPY --from=shell-builder /build/repo/index /initramfs/var/repo/index
COPY --from=shell-builder /build/repo/packages/ /initramfs/var/repo/packages/

RUN mkdir -p /initramfs/dev /initramfs/proc /initramfs/sys /initramfs/tmp /initramfs/data && \
    chmod 1777 /initramfs/tmp && \
    mknod -m 622 /initramfs/dev/console c 5 1 && \
    cd /initramfs && \
    find . | cpio -o -H newc | gzip -9 > /initramfs.cpio.gz

# ---------- Stage 4: bootable ISO (GRUB, BIOS) ----------
FROM alpine:3.20 AS iso-builder

RUN apk add --no-cache grub grub-bios xorriso

COPY boot/grub.cfg /iso/boot/grub/grub.cfg
COPY --from=kernel-builder /kernel/arch/x86/boot/bzImage /iso/boot/bzImage
COPY --from=initramfs-builder /initramfs.cpio.gz /iso/boot/initramfs.cpio.gz
RUN grub-mkrescue -o /ec-distro.iso /iso

# ---------- Output: docker build --target artifacts --output type=local,dest=out . ----------
FROM scratch AS artifacts
COPY --from=kernel-builder /kernel/arch/x86/boot/bzImage /bzImage
COPY --from=initramfs-builder /initramfs.cpio.gz /initramfs.cpio.gz
COPY --from=iso-builder /ec-distro.iso /ec-distro.iso

# ---------- Disk tools: used by ecdistro.py via docker run ----------
FROM alpine:3.20 AS tools
RUN apk add --no-cache e2fsprogs qemu-img
