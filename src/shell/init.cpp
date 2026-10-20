#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../header/init.h"

static void mountFs(const char* src, const char* target, const char* type) {
    mkdir(target, 0755);                                    
    if (mount(src, target, type, 0, nullptr) != 0 && errno != EBUSY)
        std::cerr << "init: mount " << target << ": " << std::strerror(errno) << '\n'; //This is before shell, so no print buffer
}


void initSystem() {
    mountFs("proc",     "/proc", "proc");
    mountFs("sysfs",    "/sys",  "sysfs");
    mountFs("devtmpfs", "/dev",  "devtmpfs");
}

void readOrphans() {
    while (waitpid(-1, nullptr, WNOHANG) > 0) {}            // returns 0 or -1 when none are left
}

[[noreturn]] void powerOff() {
    std::cout << std::flush;
    sync();
    reboot(RB_POWER_OFF);
    for (;;) pause();                                       // never fall out of PID 1
}

