# EC-Distro top-level Makefile.
#
#   make build   build GCC + userland + initramfs, fetch prebuilt Alpine kernel (needs docker);
#                (needs docker); artifacts are exported to out/
#   make iso     build (if needed), then make out/ecd.iso from the compiled kernel
#                and initramfs (needs grub-mkrescue + xorriso + mtools on the host)
#   make test    boot the ISO in QEMU headless and check that the kernel starts
#                /init (ecsh)
#   make run     boot the ISO in QEMU interactively (serial console in terminal)
#   make clean   remove out/ and the userland build tree
#
# Override: make build GCC_VERSION=15.2.0 ALPINE_RELEASE=v3.24 JOBS=8

OUT            ?= out
ISO            ?= $(OUT)/ecd.iso
ISODIR         := $(OUT)/iso
DOCKER         ?= docker
IMAGE          ?= ecd-export
QEMU           ?= qemu-system-x86_64
QEMU_MEM       ?= 1G
TEST_TIMEOUT   ?= 90
TEST_LOG       := $(OUT)/qemu-test.log
BOOT_MARKER    ?= Run /init as init process

BUILD_ARGS := $(if $(GCC_VERSION),--build-arg GCC_VERSION=$(GCC_VERSION)) \
              $(if $(ALPINE_RELEASE),--build-arg ALPINE_RELEASE=$(ALPINE_RELEASE)) \
              $(if $(JOBS),--build-arg JOBS=$(JOBS))

# Use KVM when available
QEMU_ACCEL := $(if $(shell test -r /dev/kvm -a -w /dev/kvm && echo y),-enable-kvm -cpu host,)

.PHONY: all build iso test run clean
all: iso

build: $(OUT)/.built

$(OUT)/.built: BuildDistro/Dockerfile BuildDistro/userland.mk $(shell find Userlandsource -type f 2>/dev/null)
	$(DOCKER) build -f BuildDistro/Dockerfile --target export $(BUILD_ARGS) -t $(IMAGE) .
	@mkdir -p $(OUT)
	@id=$$($(DOCKER) create $(IMAGE) /none) && \
	 $(DOCKER) cp $$id:/. $(OUT)/ ; rc=$$?; $(DOCKER) rm $$id >/dev/null; exit $$rc
	@touch $@

iso: $(ISO)

$(ISO): $(OUT)/.built
	@command -v grub-mkrescue >/dev/null || { echo "grub-mkrescue not found (apt install grub-pc-bin grub-common xorriso mtools)"; exit 1; }
	@kernel=$$(ls $(OUT)/boot/vmlinuz-* 2>/dev/null | head -n1); \
	 [ -n "$$kernel" ] || { echo "no kernel found in $(OUT)/boot"; exit 1; }; \
	 [ -f $(OUT)/boot/initramfs.cpio.gz ] || { echo "no initramfs found in $(OUT)/boot"; exit 1; }; \
	 mkdir -p $(ISODIR)/boot/grub; \
	 cp "$$kernel" $(ISODIR)/boot/vmlinuz; \
	 cp $(OUT)/boot/initramfs.cpio.gz $(ISODIR)/boot/initramfs.cpio.gz; \
	 for f in KERNEL_VERSION GCC_VERSION; do cp $(OUT)/$$f $(ISODIR)/ 2>/dev/null || true; done; \
	 printf '%s\n' \
	   'set timeout=0' \
	   'set default=0' \
	   'serial --unit=0 --speed=115200' \
	   'terminal_input serial console' \
	   'terminal_output serial console' \
	   'menuentry "EC-Distro" {' \
	   '    linux /boot/vmlinuz console=tty0 console=ttyS0,115200 rdinit=/init' \
	   '    initrd /boot/initramfs.cpio.gz' \
	   '}' > $(ISODIR)/boot/grub/grub.cfg; \
	 grub-mkrescue -o $@ $(ISODIR) 2>&1 | tail -n 3
	@echo "ISO: $@"

# Headless boot; passes when the kernel hands off to /init (ecsh) before the timeout.
test: $(ISO)
	@command -v $(QEMU) >/dev/null || { echo "$(QEMU) not found"; exit 1; }
	@echo "Booting $(ISO) in QEMU (timeout $(TEST_TIMEOUT)s)..."
	@: > $(TEST_LOG); \
	 $(QEMU) $(QEMU_ACCEL) -m $(QEMU_MEM) -cdrom $(ISO) -boot d \
		-display none -serial file:$(TEST_LOG) -monitor none -no-reboot & pid=$$!; \
	 result=1; \
	 for i in $$(seq 1 $(TEST_TIMEOUT)); do \
	   if grep -q "$(BOOT_MARKER)" $(TEST_LOG); then result=0; break; fi; \
	   if grep -q "Kernel panic" $(TEST_LOG); then break; fi; \
	   kill -0 $$pid 2>/dev/null || break; \
	   sleep 1; done; \
	 kill $$pid 2>/dev/null; wait $$pid 2>/dev/null; \
	 if [ $$result -eq 0 ]; then echo "PASS: kernel booted and started /init"; \
	 else echo "FAIL: boot marker not seen; last output:"; tail -n 20 $(TEST_LOG); fi; \
	 exit $$result

run: $(ISO)
	$(QEMU) $(QEMU_ACCEL) -m $(QEMU_MEM) -cdrom $(ISO) -boot d -nographic

clean:
	rm -rf $(OUT) BuildDistro/build
