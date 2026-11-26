#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../header/init.h"

// Virtual disk attached by ecdistro.py (QEMU virtio drive), formatted as ext2.
static const char* const DATA_DEV   = "/dev/vda";
static const char* const DATA_MOUNT = "/data";

static bool mountFs(const char* src, const char* target, const char* type) {
    mkdir(target, 0755);
    if (mount(src, target, type, 0, NULL) != 0 && errno != EBUSY) {
        fprintf(stderr, "init: mount %s: %s\n", target, strerror(errno)); // This is before shell, so no print buffer
        return false;
    }
    return true;
}

// Mounts the virtual disk at /data if one is attached. Without a disk the
// system runs from RAM only, as before.
static void mountData(void) {
    struct stat st;
    if (stat(DATA_DEV, &st) != 0 || !S_ISBLK(st.st_mode)) return;

    if (mountFs(DATA_DEV, DATA_MOUNT, "ext2"))
        printf("init: %s mounted at %s\n", DATA_DEV, DATA_MOUNT);
}

void initSystem(void) {
    mountFs("proc",     "/proc", "proc");
    mountFs("sysfs",    "/sys",  "sysfs");
    mountFs("devtmpfs", "/dev",  "devtmpfs");
    mountData();
}

void readOrphans(void) {
    while (waitpid(-1, NULL, WNOHANG) > 0) {}               // returns 0 or -1 when none are left
}

_Noreturn void powerOff(void) {
    fflush(stdout);
    sync();
    umount(DATA_MOUNT);                                     // fails harmlessly when no disk is mounted
    reboot(RB_POWER_OFF);
    for (;;) pause();                                       // never fall out of PID 1
}
