#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../header/init.h"

// Virtual disk attached by ecdistro.py (QEMU virtio drive), formatted as ext2.
static constexpr const char* DATA_DEV   = "/dev/vda";
static constexpr const char* DATA_MOUNT = "/data";

static bool mountFs(const char* src, const char* target, const char* type) {
    mkdir(target, 0755);
    if (mount(src, target, type, 0, nullptr) != 0 && errno != EBUSY) {
        std::cerr << "init: mount " << target << ": " << std::strerror(errno) << '\n'; //This is before shell, so no print buffer
        return false;
    }
    return true;
}

// Mounts the virtual disk at /data if one is attached. Without a disk the
// system runs from RAM only, as before.
static void mountData() {
    struct stat st;
    if (stat(DATA_DEV, &st) != 0 || !S_ISBLK(st.st_mode)) return;

    if (mountFs(DATA_DEV, DATA_MOUNT, "ext2"))
        std::cout << "init: " << DATA_DEV << " mounted at " << DATA_MOUNT << '\n';
}

void initSystem() {
    mountFs("proc",     "/proc", "proc");
    mountFs("sysfs",    "/sys",  "sysfs");
    mountFs("devtmpfs", "/dev",  "devtmpfs");
    mountData();
}

void readOrphans() {
    while (waitpid(-1, nullptr, WNOHANG) > 0) {}            // returns 0 or -1 when none are left
}

[[noreturn]] void powerOff() {
    std::cout << std::flush;
    sync();
    umount(DATA_MOUNT);                                     // fails harmlessly when no disk is mounted
    reboot(RB_POWER_OFF);
    for (;;) pause();                                       // never fall out of PID 1
}
