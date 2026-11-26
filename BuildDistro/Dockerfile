# syntax=docker/dockerfile:1
#
# EC-Distro build image.
#   Stage 1 (gcc):      builds the latest GCC release from source.
#   Stage 2 (userland): builds Userlandsource with that GCC via userland.mk.
#   Stage 3 (kernel):   fetches the latest stable Linux kernel, builds it with
#                       that GCC, copies the GCC toolchain and the userland tools
#                       (bin/) into the kernel files, and packs an initramfs whose
#                       /init (PID 1) is the shell (ecsh, from main.cpp).
#   Stage 4 (export):   minimal stage holding only the artifacts.
#
# The build context must be the repository root (it needs Userlandsource/):
#   docker build -f BuildDistro/Dockerfile -t ecd-build .
#   docker build -f BuildDistro/Dockerfile --target export --output type=local,dest=./out .
#
# Pin versions instead of "latest" with:
#   --build-arg GCC_VERSION=15.2.0 --build-arg KERNEL_VERSION=6.12.5

ARG BASE_IMAGE=debian:bookworm-slim

# ---------------------------------------------------------------- GCC
FROM ${BASE_IMAGE} AS gcc

ARG GCC_VERSION=latest
ARG JOBS=0

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential ca-certificates curl xz-utils bzip2 file \
        libgmp-dev libmpfr-dev libmpc-dev libisl-dev zlib1g-dev flex bison \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

RUN set -eux; \
    if [ "$GCC_VERSION" = "latest" ]; then \
        GCC_VERSION="$(curl -fsSL https://ftp.gnu.org/gnu/gcc/ \
            | grep -oE 'gcc-[0-9]+\.[0-9]+\.[0-9]+/' | tr -d '/' | sed 's/^gcc-//' \
            | sort -V | tail -n1)"; \
    fi; \
    echo "$GCC_VERSION" > /gcc-version; \
    curl -fsSLO "https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.xz"; \
    tar -xf "gcc-${GCC_VERSION}.tar.xz"; \
    rm "gcc-${GCC_VERSION}.tar.xz"; \
    mv "gcc-${GCC_VERSION}" gcc

WORKDIR /src/build
RUN set -eux; \
    n="${JOBS}"; [ "$n" -gt 0 ] || n="$(nproc)"; \
    ../gcc/configure \
        --prefix=/opt/gcc \
        --enable-languages=c,c++ \
        --disable-multilib \
        --disable-bootstrap \
        --with-system-zlib; \
    make -j"$n"; \
    make install-strip

# ------------------------------------------------------------- Userland
FROM ${BASE_IMAGE} AS userland

ARG JOBS=0

RUN apt-get update && apt-get install -y --no-install-recommends make binutils libc6-dev \
        libgmp10 libmpfr6 libmpc3 libisl23 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=gcc /opt/gcc /opt/gcc
ENV PATH=/opt/gcc/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/gcc/lib64:/opt/gcc/lib

# Keep the repo layout: userland.mk refers to ../Userlandsource
COPY Userlandsource /work/Userlandsource
COPY BuildDistro/userland.mk /work/BuildDistro/userland.mk

WORKDIR /work/BuildDistro
RUN set -eux; \
    n="${JOBS}"; [ "$n" -gt 0 ] || n="$(nproc)"; \
    make -f userland.mk -j"$n" CXX=/opt/gcc/bin/g++ STATIC=1

# --------------------------------------------------------------- Kernel
# Prebuilt minimal kernel (Alpine "virt" flavour) instead of compiling Linux.
FROM ${BASE_IMAGE} AS kernel

ARG ALPINE_RELEASE=latest-stable
ARG KERNEL_URL=https://dl-cdn.alpinelinux.org/alpine/${ALPINE_RELEASE}/releases/x86_64/netboot/vmlinuz-virt

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates curl cpio gzip \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /out/boot /out/rootfs/bin \
    && curl -fsSL "$KERNEL_URL" -o /out/boot/vmlinuz-virt \
    && echo "alpine-${ALPINE_RELEASE}-virt" > /out/KERNEL_VERSION

# Userland (built with the GCC from the first stage)
COPY --from=userland /work/BuildDistro/build/rootfs/bin /userland-bin

# initramfs: the shell is /init so the kernel runs it as PID 1
RUN set -eux; \
    r=/initramfs; \
    mkdir -p $r/bin $r/dev $r/proc $r/sys $r/tmp $r/etc $r/root; \
    cp /userland-bin/* $r/bin/; \
    cp /userland-bin/ecsh $r/init; \
    chmod 755 $r/init $r/bin/*; \
    cp /userland-bin/* /out/rootfs/bin/; \
    (cd $r && find . | cpio -o -H newc --owner=0:0 | gzip -9) > /out/boot/initramfs.cpio.gz

# The compiled GCC is the only toolchain shipped
COPY --from=gcc /opt/gcc /out/rootfs/usr/local/gcc
COPY --from=gcc /gcc-version /out/GCC_VERSION

# --------------------------------------------------------------- Export
FROM scratch AS export
COPY --from=kernel /out /

# Default target
FROM kernel
