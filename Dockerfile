# ---------- Etapa 1: shell + tools estáticos ----------
FROM alpine:3.20 AS shell-builder

RUN apk add --no-cache build-base make findutils

WORKDIR /build
COPY src/ ./src/
COPY main.cpp makefile ./
RUN make

# ---------- Etapa 2: kernel mínimo ----------
FROM alpine:3.20 AS kernel-builder

RUN apk add --no-cache \
    build-base linux-headers bc flex bison perl bash \
    openssl-dev elfutils-dev curl xz diffutils findutils

# Verifica la versión vigente en kernel.org
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
RUN mkdir -p /initramfs/dev /initramfs/proc /initramfs/sys && \
    mknod -m 622 /initramfs/dev/console c 5 1 && \
    cd /initramfs && \
    find . | cpio -o -H newc | gzip -9 > /initramfs.cpio.gz

# ---------- Etapa 4: artefactos de arranque (QEMU/VM/hardware) ----------
# docker build --target artifacts --output type=local,dest=./out .
FROM scratch AS artifacts
COPY --from=kernel-builder /kernel/arch/x86/boot/bzImage /bzImage
COPY --from=initramfs-builder /initramfs.cpio.gz /initramfs.cpio.gz

# ---------- Etapa 5: contenedor de prueba (última etapa = default) ----------
FROM scratch AS runtime
COPY --from=shell-builder /build/bin/shell /shell
COPY --from=shell-builder /build/bin/tools/ /bin/
ENV PATH=/bin
ENTRYPOINT ["/shell"]