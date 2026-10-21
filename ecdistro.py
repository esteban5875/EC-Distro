#!/usr/bin/env python3
"""Build EC-Distro in Docker and boot it in QEMU.

  python3 ecdistro.py              build, ask for a disk size, boot
  python3 ecdistro.py build        build out/ec-distro-<version>.iso
  python3 ecdistro.py run          boot the ISO with a fresh virtual disk
  python3 ecdistro.py test         headless boot check (used by CI)

All build dependencies live in Docker. The host needs Python 3.8+, Docker
(with BuildKit, i.e. Docker 23+) and, to boot, qemu-system-x86_64.

The virtual disk is throwaway: it is created right before QEMU starts,
mounted by the guest at /data, and deleted when QEMU exits.
"""

import argparse
import os
import re
import select
import shlex
import shutil
import signal
import subprocess
import sys
import time
from contextlib import contextmanager
from pathlib import Path

ROOT = Path(__file__).resolve().parent
VERSION = (ROOT / "VERSION").read_text().strip()
TOOLS_IMAGE = "ec-distro-tools:" + VERSION

DEFAULT_DISK_SIZE = "512M"
MIN_DISK_MIB = 16
MAX_DISK_MIB = 64 * 1024
DISK_NAME = "disk.qcow2"
COMMANDS = ("build", "run", "test", "all")


# ---------- helpers ----------

def info(msg):
    print("==> " + msg, flush=True)


def die(msg):
    print("ecdistro: error: " + msg, file=sys.stderr, flush=True)
    sys.exit(1)


def run_cmd(argv, what):
    print("$ " + shlex.join(argv), flush=True)
    rc = subprocess.run(argv).returncode
    if rc != 0:
        die("{} failed (exit code {})".format(what, rc))


def docker(args):
    return (["sudo"] if args.sudo else []) + ["docker"]


def check_docker(args):
    if not shutil.which("docker"):
        die("docker not found. Install Docker: https://docs.docker.com/engine/install/")
    res = subprocess.run(docker(args) + ["info"], stdout=subprocess.DEVNULL,
                         stderr=subprocess.PIPE, text=True)
    if res.returncode == 0:
        return
    if "permission denied" in res.stderr.lower():
        die("no permission to use Docker. Either add yourself to the docker group\n"
            "  (sudo usermod -aG docker $USER, then log in again) or pass --sudo.")
    die("cannot reach the Docker daemon. Is it running?\n" + res.stderr.strip())


def check_qemu():
    if not shutil.which("qemu-system-x86_64"):
        die("qemu-system-x86_64 not found. Install QEMU, e.g.\n"
            "  Debian/Ubuntu: sudo apt install qemu-system-x86\n"
            "  Fedora:        sudo dnf install qemu-system-x86\n"
            "  Arch:          sudo pacman -S qemu-full")


def out_dir(args):
    return Path(args.out).resolve()


def iso_path(args):
    if getattr(args, "iso", None):
        return Path(args.iso).resolve()
    return out_dir(args) / "ec-distro-{}.iso".format(VERSION)


def require_iso(args):
    iso = iso_path(args)
    if not iso.is_file():
        die("{} not found. Run `python3 ecdistro.py build` first.".format(iso))
    return iso


# ---------- disk size ----------

def parse_size(text):
    """'512M', '2G', '2GB', '2GiB' -> ('512M', 512). Raises ValueError."""
    m = re.fullmatch(r"\s*(\d+)\s*([MG])(?:I?B)?\s*", text.upper())
    if not m:
        raise ValueError("'{}' is not a size like 512M or 2G".format(text.strip()))
    num, unit = int(m.group(1)), m.group(2)
    mib = num * (1024 if unit == "G" else 1)
    if mib < MIN_DISK_MIB:
        raise ValueError("disk must be at least {}M".format(MIN_DISK_MIB))
    if mib > MAX_DISK_MIB:
        raise ValueError("disk must be at most {}G".format(MAX_DISK_MIB // 1024))
    return "{}{}".format(num, unit), mib


def ask_disk_size(given):
    if given:
        try:
            return parse_size(given)[0]
        except ValueError as e:
            die(str(e))
    if not sys.stdin.isatty():
        die("no disk size given. Pass --disk-size, e.g. --disk-size 512M")
    while True:
        try:
            answer = input("Virtual disk size [{}]: ".format(DEFAULT_DISK_SIZE))
        except EOFError:
            die("no disk size given")
        try:
            return parse_size(answer or DEFAULT_DISK_SIZE)[0]
        except ValueError as e:
            print("  " + str(e))


# ---------- virtual disk ----------

# Runs inside the tools image. $1 = size, $2 = file name in /out.
# truncate makes a sparse raw file, mkfs.ext2 formats it, qemu-img
# converts it to qcow2 (which only stores used blocks).
DISK_SCRIPT = (
    'set -e; raw="/out/$2.raw"; trap \'rm -f "$raw"\' EXIT; '
    'truncate -s "$1" "$raw"; '
    'mkfs.ext2 -q -F -L ecdata "$raw"; '
    'qemu-img convert -O qcow2 "$raw" "/out/$2"'
)


def create_disk(args, size):
    out = out_dir(args)
    disk = out / DISK_NAME
    probe = subprocess.run(docker(args) + ["image", "inspect", TOOLS_IMAGE],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if probe.returncode != 0:
        die("Docker image {} not found. Run `python3 ecdistro.py build` first."
            .format(TOOLS_IMAGE))
    if disk.exists():
        info("Removing leftover disk from an earlier run: " + str(disk))
        disk.unlink()

    user = []
    if hasattr(os, "getuid"):
        # Create the file as the calling user, so this script can delete it.
        user = ["--user", "{}:{}".format(os.getuid(), os.getgid())]
    info("Creating {} virtual disk (ext2, qcow2)".format(size))
    run_cmd(docker(args) + ["run", "--rm"] + user +
            ["--mount", "type=bind,src={},dst=/out".format(out),
             TOOLS_IMAGE, "sh", "-c", DISK_SCRIPT, "sh", size, DISK_NAME],
            "creating the virtual disk")
    return disk


def _raise_exit(signum, _frame):
    raise SystemExit(128 + signum)


@contextmanager
def throwaway_disk(args, size):
    """Creates the disk, yields its path, and always deletes it afterwards."""
    old = {}
    for sig in (signal.SIGTERM, getattr(signal, "SIGHUP", None)):
        if sig is not None:
            old[sig] = signal.signal(sig, _raise_exit)   # so `finally` runs
    disk = out_dir(args) / DISK_NAME
    try:
        yield create_disk(args, size)
    finally:
        if disk.exists():   # also catches a half-written disk if creation failed
            disk.unlink()
            info("Deleted virtual disk " + str(disk))
        for sig, handler in old.items():
            signal.signal(sig, handler)


# ---------- QEMU ----------

def qemu_argv(args, iso, disk):
    argv = ["qemu-system-x86_64", "-m", args.memory,
            "-cdrom", str(iso), "-boot", "d",
            "-nographic", "-no-reboot"]
    if disk is not None:
        # Commas in a -drive value must be doubled.
        argv += ["-drive", "file={},if=virtio,format=qcow2".format(str(disk).replace(",", ",,"))]
    if not args.no_kvm and os.access("/dev/kvm", os.R_OK | os.W_OK):
        argv += ["-enable-kvm", "-cpu", "host"]
    return argv


# ---------- commands ----------

def cmd_build(args):
    check_docker(args)
    out = out_dir(args)
    out.mkdir(parents=True, exist_ok=True)

    extra = list(args.docker_arg)
    if args.no_cache:
        extra.append("--no-cache")
    if args.kernel_version:
        print("note: no pinned checksum for kernel {}; the download is not verified"
              .format(args.kernel_version))
        extra += ["--build-arg", "KERNEL_VERSION=" + args.kernel_version,
                  "--build-arg", "KERNEL_SHA256="]

    info("Building kernel, initramfs and ISO (the first build takes a few minutes)")
    run_cmd(docker(args) + ["build", "--target", "artifacts",
                            "--output", "type=local,dest=" + str(out)] + extra + [str(ROOT)],
            "docker build (artifacts)")
    os.replace(str(out / "ec-distro.iso"), str(iso_path(args)))

    info("Building the disk tools image " + TOOLS_IMAGE)
    run_cmd(docker(args) + ["build", "--target", "tools", "-t", TOOLS_IMAGE] + extra + [str(ROOT)],
            "docker build (tools)")

    info("Build finished:")
    for f in (iso_path(args), out / "bzImage", out / "initramfs.cpio.gz"):
        print("    {:<45} {:>8.1f} KiB".format(str(f), f.stat().st_size / 1024))


def cmd_run(args):
    check_qemu()
    iso = require_iso(args)
    size = ask_disk_size(args.disk_size)
    check_docker(args)
    with throwaway_disk(args, size) as disk:
        info("Booting EC-Distro. Type `exit` to power off; Ctrl-A then X kills QEMU.")
        argv = qemu_argv(args, iso, disk)
        print("$ " + shlex.join(argv), flush=True)
        rc = subprocess.run(argv).returncode
    if rc != 0:
        die("QEMU exited with code {}".format(rc))


def cmd_all(args):
    # Ask for the size first, so the user isn't prompted after a long build.
    args.disk_size = ask_disk_size(args.disk_size)
    cmd_build(args)
    cmd_run(args)


class Console:
    """Minimal expect-style driver for the guest's serial console."""

    def __init__(self, proc, echo):
        self.proc = proc
        self.fd = proc.stdout.fileno()
        self.echo = echo
        self.buf = ""
        self.pos = 0

    def expect(self, pattern, timeout):
        rx = re.compile(pattern, re.MULTILINE)
        deadline = time.monotonic() + timeout
        while True:
            m = rx.search(self.buf, self.pos)
            if m:
                self.pos = m.end()
                return
            left = deadline - time.monotonic()
            if left <= 0:
                raise RuntimeError("timed out waiting for {!r}".format(pattern))
            ready, _, _ = select.select([self.fd], [], [], left)
            if not ready:
                continue
            chunk = os.read(self.fd, 4096)
            if not chunk:
                raise RuntimeError("QEMU exited while waiting for {!r}".format(pattern))
            text = chunk.decode("utf-8", "replace")
            self.buf += text
            if self.echo:
                sys.stdout.write(text)
                sys.stdout.flush()

    def send(self, line):
        self.proc.stdin.write((line + "\n").encode())
        self.proc.stdin.flush()


def cmd_test(args):
    check_qemu()
    iso = require_iso(args)
    check_docker(args)
    steps = [
        # (text to send or None, regex to wait for)
        (None, r"/dev/vda mounted at /data"),
        (None, r"\$ "),
        ("ls /data", r"lost\+found"),
        ("echo ec-smoke-ok", r"^ec-smoke-ok\r?$"),
        ("exit", r"Bye"),
    ]
    with throwaway_disk(args, "32M") as disk:
        argv = qemu_argv(args, iso, disk)
        print("$ " + shlex.join(argv), flush=True)
        proc = subprocess.Popen(argv, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        con = Console(proc, args.verbose)
        deadline = time.monotonic() + args.timeout
        try:
            for send, pattern in steps:
                if send is not None:
                    con.send(send)
                con.expect(pattern, max(1.0, deadline - time.monotonic()))
                print("  ok: " + (send or pattern), flush=True)
            rc = proc.wait(timeout=max(1.0, deadline - time.monotonic()))
        except (RuntimeError, subprocess.TimeoutExpired) as e:
            proc.kill()
            proc.wait()
            print("---- guest output (last 3000 chars) ----\n" + con.buf[-3000:])
            die("boot test failed: {}".format(e))
    if rc != 0:
        die("boot test failed: QEMU exited with code {}".format(rc))
    info("Boot test passed")


# ---------- CLI ----------

def build_parser():
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("--out", default=str(ROOT / "out"),
                        help="output directory (default: ./out)")
    common.add_argument("--sudo", action="store_true",
                        help="run docker through sudo")

    build = argparse.ArgumentParser(add_help=False)
    build.add_argument("--kernel-version",
                       help="Linux version to build instead of the pinned one (checksum not verified)")
    build.add_argument("--no-cache", action="store_true", help="rebuild every Docker stage")
    build.add_argument("--docker-arg", action="append", default=[], metavar="ARG",
                       help="extra argument for docker build, e.g. --docker-arg=--network=host "
                            "(repeatable)")

    boot = argparse.ArgumentParser(add_help=False)
    boot.add_argument("--memory", default="256M", help="guest RAM (default: 256M)")
    boot.add_argument("--no-kvm", action="store_true", help="don't use KVM even if available")
    boot.add_argument("--iso", help="ISO to boot (default: out/ec-distro-<version>.iso)")

    disk = argparse.ArgumentParser(add_help=False)
    disk.add_argument("--disk-size",
                      help="virtual disk size, e.g. 512M or 2G (asked if omitted)")

    p = argparse.ArgumentParser(
        prog="ecdistro.py",
        description="Build EC-Distro in Docker and boot it in QEMU. "
                    "With no command, runs `all`.")
    p.add_argument("--version", action="version", version="EC-Distro " + VERSION)
    sub = p.add_subparsers(dest="command", metavar="{build,run,test,all}")

    sp = sub.add_parser("build", parents=[common, build], help="build the ISO in Docker")
    sp.set_defaults(func=cmd_build)
    sp = sub.add_parser("run", parents=[common, boot, disk],
                        help="boot the ISO with a fresh virtual disk (deleted on exit)")
    sp.set_defaults(func=cmd_run)
    sp = sub.add_parser("test", parents=[common, boot], help="headless boot check")
    sp.add_argument("--timeout", type=float, default=180, help="seconds (default: 180)")
    sp.add_argument("--verbose", action="store_true", help="show the guest console")
    sp.set_defaults(func=cmd_test)
    sp = sub.add_parser("all", parents=[common, build, boot, disk], help="build, then run (default)")
    sp.set_defaults(func=cmd_all)
    return p


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    if not argv or (argv[0] not in COMMANDS and argv[0] not in ("-h", "--help", "--version")):
        argv.insert(0, "all")
    args = build_parser().parse_args(argv)
    try:
        args.func(args)
    except KeyboardInterrupt:
        die("interrupted")


if __name__ == "__main__":
    main()
