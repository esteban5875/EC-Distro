FROM alpine:3.20 AS shell-builder

RUN apk add --no-cache build-base make findutils

WORKDIR /build
COPY src/ ./src/
COPY main.cpp makefile ./
RUN make

FROM alpine:3.20 AS kernel-builder

RUN apk add --no-cache \
    build-base linux-headers bc flex bison perl bash \
    openssl-dev elfutils-dev curl xz diffutils findutils

ARG KERNEL_VERSION=6.12.1
WORKDIR /kernel
RUN curl -fsSL https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-${KERNEL_VERSION}.tar.xz \
    | tar -xJ --strip-components=1

RUN make tinyconfig && \
    scripts/config \
      -e 64BIT \
      -e BLK_DEV_INITRD -e RD_GZIP \
      -e BINFMT_ELF -e BINFMT_SCRIPT \
      -e PRINTK -e TTY \
      -e SERIAL_8250 -e SERIAL_8250_CONSOLE \
      -e DEVTMPFS -e DEVTMPFS_MOUNT \
      -e PROC_FS -e SYSFS && \
    make olddefconfig && \
    make -j"$(nproc)" bzImage

# ---------- Etapa 3: initramfs ----------
FROM alpine:3.20 AS initramfs-builder

RUN apk add --no-cache cpio gzip

COPY --from=shell-builder /build/bin/shell /initramfs/init
COPY --from=shell-builder /build/bin/tools/ /initramfs/bin/

COPY repo/index /initramfs/var/repo/index
COPY repo/packages/ /initramfs/var/repo/packages/

RUN mkdir -p /initramfs/dev /initramfs/proc /initramfs/sys && \
    mknod -m 622 /initramfs/dev/console c 5 1 && \
    cd /initramfs && \
    find . | cpio -o -H newc | gzip -9 > /initramfs.cpio.gz

FROM scratch AS artifacts
COPY --from=kernel-builder /kernel/arch/x86/boot/bzImage /bzImage
COPY --from=initramfs-builder /initramfs.cpio.gz /initramfs.cpio.gz