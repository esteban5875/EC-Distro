#include "./header/pkg.h"
#include "./header/common.h"

#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

//Static funcs for installation, three steps: unpack tarball, move to /bin, chmod 755. Returns true on success, false on failure.

// Read exactly n bytes; false on EOF or error.
static bool readFull(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t r = read(fd, buf, n);
        if (r < 0 && errno == EINTR) continue;
        if (r <= 0) return false;
        buf += r;
        n -= r;
    }
    return true;
}

static bool writeFull(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t w = write(fd, buf, n);
        if (w < 0 && errno == EINTR) continue;
        if (w <= 0) return false;
        buf += w;
        n -= w;
    }
    return true;
}

// Tar numeric fields are NUL/space-terminated octal strings.
static size_t parseOctal(const char *p, size_t len) {
    size_t v = 0;
    for (size_t i = 0; i < len && p[i] >= '0' && p[i] <= '7'; i++)
        v = v * 8 + (p[i] - '0');
    return v;
}

// mkdir -p for every parent directory of `path`.
static bool makeParents(const std::string &path) {
    for (size_t i = 1; i < path.size(); i++) {
        if (path[i] != '/') continue;
        std::string dir = path.substr(0, i);
        if (mkdir(dir.c_str(), 0755) < 0 && errno != EEXIST) return false;
    }
    return true;
}

// True if the last INSTALL_FLAG_LEN bytes of the open tarball are INSTALL_FLAG.
// Moves the file offset.
static bool tarballHasFlag(int fd, off_t fileSize) {
    if (fileSize < static_cast<off_t>(INSTALL_FLAG_LEN)) return false;
    if (lseek(fd, fileSize - INSTALL_FLAG_LEN, SEEK_SET) < 0) return false;

    char tail[INSTALL_FLAG_LEN];
    if (!readFull(fd, tail, INSTALL_FLAG_LEN)) return false;
    return memcmp(tail, INSTALL_FLAG, INSTALL_FLAG_LEN) == 0;
}

// Appends INSTALL_FLAG to the end of the tarball to mark it installed.
static bool appendInstallFlag(const std::string &tarPath) {
    int fd = open(tarPath.c_str(), O_WRONLY | O_APPEND);
    if (fd < 0) {
        errLine("pkg: cannot open '%s': %s", tarPath.c_str(), strerror(errno));
        return false;
    }
    bool ok = writeFull(fd, INSTALL_FLAG, INSTALL_FLAG_LEN);
    if (close(fd) < 0) ok = false;
    if (!ok) errLine("pkg: cannot write install flag to '%s'", tarPath.c_str());
    return ok;
}

// Truncates INSTALL_FLAG off the end of the tarball to mark it not installed.
// Does nothing if the flag isn't there.
static bool stripInstallFlag(const std::string &tarPath) {
    int fd = open(tarPath.c_str(), O_RDWR);
    if (fd < 0) {
        errLine("pkg: cannot open '%s': %s", tarPath.c_str(), strerror(errno));
        return false;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        errLine("pkg: cannot stat '%s': %s", tarPath.c_str(), strerror(errno));
        close(fd);
        return false;
    }

    bool ok = true;
    if (tarballHasFlag(fd, st.st_size) &&
        ftruncate(fd, st.st_size - INSTALL_FLAG_LEN) < 0) {
        errLine("pkg: cannot remove install flag from '%s': %s", tarPath.c_str(), strerror(errno));
        ok = false;
    }
    if (close(fd) < 0) ok = false;
    return ok;
}

// Extracts an uncompressed (ustar/GNU) tar into destDir.
// Only regular files and directories are extracted; a trailing
// INSTALL_FLAG is ignored.
static bool unpackTarball(const std::string &tarPath, const std::string &destDir) {
    int fd = open(tarPath.c_str(), O_RDONLY);
    if (fd < 0) {
        errLine("pkg: cannot open '%s': %s", tarPath.c_str(), strerror(errno));
        return false;
    }

    // Stop reading before the install flag, if the tarball has one.
    struct stat st;
    if (fstat(fd, &st) < 0) {
        errLine("pkg: cannot stat '%s': %s", tarPath.c_str(), strerror(errno));
        close(fd);
        return false;
    }
    off_t left = st.st_size;
    if (tarballHasFlag(fd, st.st_size)) left -= INSTALL_FLAG_LEN;
    if (lseek(fd, 0, SEEK_SET) < 0) { close(fd); return false; }

    char header[512];
    char buf[512];
    bool ok = true;

    while (left >= 512 && readFull(fd, header, sizeof(header))) {
        left -= 512;
        if (header[0] == '\0') break;   // zero block = end of archive

        std::string name(header, strnlen(header, 100));
        std::string prefix(header + 345, strnlen(header + 345, 155));
        if (!prefix.empty()) name = prefix + "/" + name;

        size_t size   = parseOctal(header + 124, 12);
        char   type   = header[156];
        size_t blocks = (size + 511) / 512;

        // Entry data can't run into the flag (or past the end of the file).
        if (static_cast<off_t>(blocks * 512) > left) {
            errLine("pkg: truncated tarball '%s'", tarPath.c_str());
            ok = false;
            break;
        }
        left -= blocks * 512;

        // Refuse paths that could escape destDir.
        if (name.empty() || name[0] == '/' || name.find("..") != std::string::npos) {
            errLine("pkg: unsafe path in tarball: '%s'", name.c_str());
            ok = false;
            break;
        }

        std::string out = joinPath(destDir, name);

        if (type == '5') {                          // directory
            if (!makeParents(out + "/")) { ok = false; break; }
        } else if (type == '0' || type == '\0') {   // regular file
            if (!makeParents(out)) { ok = false; break; }

            int ofd = open(out.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (ofd < 0) {
                errLine("pkg: cannot create '%s': %s", out.c_str(), strerror(errno));
                ok = false;
                break;
            }

            size_t left = size;
            for (size_t i = 0; i < blocks; i++) {
                if (!readFull(fd, buf, sizeof(buf))) { ok = false; break; }
                size_t n = left < sizeof(buf) ? left : sizeof(buf);
                if (!writeFull(ofd, buf, n)) { ok = false; break; }
                left -= n;
            }
            close(ofd);
            if (!ok) {
                errLine("pkg: failed extracting '%s'", out.c_str());
                break;
            }
            continue;   // data blocks already consumed
        }

        // Skip data of any entry type we don't extract.
        if (lseek(fd, blocks * 512, SEEK_CUR) < 0) { ok = false; break; }
    }

    close(fd);
    return ok;
}

// Moves srcPath to /bin/<name>. rename() can't cross filesystems,
// so on EXDEV it falls back to copy + unlink.
static bool moveToBin(const std::string &srcPath, const std::string &name) {
    std::string dest = joinPath("/bin", name);

    if (rename(srcPath.c_str(), dest.c_str()) == 0) return true;
    if (errno != EXDEV) {
        errLine("pkg: cannot move '%s' to '%s': %s",
                srcPath.c_str(), dest.c_str(), strerror(errno));
        return false;
    }

    int in = open(srcPath.c_str(), O_RDONLY);
    if (in < 0) {
        errLine("pkg: cannot open '%s': %s", srcPath.c_str(), strerror(errno));
        return false;
    }
    int out = open(dest.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) {
        errLine("pkg: cannot create '%s': %s", dest.c_str(), strerror(errno));
        close(in);
        return false;
    }

    char buf[4096];
    ssize_t n;
    bool ok = true;
    while ((n = read(in, buf, sizeof(buf))) != 0) {
        if (n < 0) {
            if (errno == EINTR) continue;
            ok = false;
            break;
        }
        if (!writeFull(out, buf, n)) { ok = false; break; }
    }
    close(in);
    close(out);

    if (!ok) {
        errLine("pkg: failed copying '%s' to '%s'", srcPath.c_str(), dest.c_str());
        unlink(dest.c_str());
        return false;
    }

    unlink(srcPath.c_str());
    return true;
}

// Sets rwxr-xr-x on path so the shell can run it.
static bool makeExecutable(const std::string &path) {
    if (chmod(path.c_str(), 0755) < 0) {
        errLine("pkg: cannot chmod '%s': %s", path.c_str(), strerror(errno));
        return false;
    }
    return true;
}

// Same format loadDB uses for tarball names: 1.0 -> "1.0", 1.5 -> "1.5".
static std::string versionString(float v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%g", v);
    std::string s = buf;
    if (s.find('.') == std::string::npos) s += ".0";
    return s;
}

static int removeEntry(const char *path, const struct stat *, int, struct FTW *) {
    return remove(path);
}

// rm -rf for the temp extraction dir.
static void removeTree(const std::string &path) {
    nftw(path.c_str(), removeEntry, 16, FTW_DEPTH | FTW_PHYS);
}

bool parseArgs(int argc, char **argv, Args &out) {
    if (argc < 2) return false;

    std::string cmd = argv[1];
    if (cmd == "list") {
        out.command = Command::List;
        out.name.clear();
    } else if (cmd == "info") {
        if (argc < 3) return false;
        out.command = Command::Info;
        out.name = argv[2];
    } else if (cmd == "install") {
        if (argc < 3) return false;
        out.command = Command::Install;
        out.name = argv[2];
    } else if (cmd == "remove") {
        if (argc < 3) return false;
        out.command = Command::Remove;
        out.name = argv[2];
    } else {
        return false;
    }

    return true;
}

bool usage() {
    printLine("Usage: pkg <command> [args]");
    printLine("Commands:");
    printLine("  list");
    printLine("  info <package_name>");
    printLine("  install <package_name>");
    printLine("  remove <package_name>");

    return true;
}

bool cmdList(const std::vector<dbEntry> &db) {
    for (const auto &entry : db) {
        printLine("%s %f", entry.name.c_str(), entry.version);
    }
    return true;
}

bool cmdInfo(const std::vector<dbEntry> &db, const std::string &name) {
    dbEntry entry;
    if (!searchPackage(db, name, entry)) {
        errLine("pkg: package '%s' not found", name.c_str());
        return false;
    }

    printLine("Name: %s", entry.name.c_str());
    printLine("Version: %f", entry.version);
    printLine("Size: %.2f bytes", entry.size);
    printLine("Installed: %s", entry.installed ? "Yes" : "No");

    return true;
}

bool cmdInstall(std::vector<dbEntry> &db, const std::string &name) {
    dbEntry found;
    if (!searchPackage(db, name, found) || found.id < 0 || static_cast<size_t>(found.id) >= db.size()) {
        errLine("pkg: package '%s' not found", name.c_str());
        return false;
    }
    // searchPackage returns a copy; ids are indexes into db, so update the real entry.
    dbEntry &entry = db[found.id];

    // loadDB sets installed from the flag at the end of the tarball.
    if (entry.installed) {
        printLine("pkg: package '%s' is already installed", name.c_str());
        return true;
    }

    std::string installedPath = joinPath("/bin", entry.name);
    struct stat st;

    std::string tarballPath = joinPath(joinPath(repo, "packages"), entry.name + "-" + versionString(entry.version) + ".tar");

    // Unique, private temp dir; avoids stale files and symlink tricks in /tmp.
    char tmpl[] = "/tmp/pkg_install_XXXXXX";
    if (!mkdtemp(tmpl)) {
        errLine("pkg: cannot create temp dir: %s", strerror(errno));
        return false;
    }
    std::string tempDir = tmpl;

    if (!unpackTarball(tarballPath, tempDir)) {
        errLine("pkg: failed to unpack tarball for '%s'", name.c_str());
        removeTree(tempDir);
        return false;
    }

    // Tarballs are packed relative to /, so the binary is at bin/<name>.
    std::string binPath = joinPath(joinPath(tempDir, "bin"), entry.name);
    if (stat(binPath.c_str(), &st) < 0 || !S_ISREG(st.st_mode)) {
        errLine("pkg: tarball for '%s' has no bin/%s", name.c_str(), entry.name.c_str());
        removeTree(tempDir);
        return false;
    }

    bool moved = moveToBin(binPath, entry.name);
    removeTree(tempDir);
    if (!moved) {
        errLine("pkg: failed to move '%s' to /bin", entry.name.c_str());
        return false;
    }

    if (!makeExecutable(installedPath)) {
        errLine("pkg: failed to set executable permissions on '%s'", installedPath.c_str());
        return false;
    }

    if (!appendInstallFlag(tarballPath)) {
        errLine("pkg: '%s' was copied to /bin but could not be marked installed", name.c_str());
        return false;
    }
    entry.installed = true;

    printLine("pkg: package '%s' installed successfully", name.c_str());
    return true;
}

bool cmdRemove(std::vector<dbEntry> &db, const std::string &name) {
    // Work from a copy only; the db is reloaded from disk once we're done.
    dbEntry entry;
    if (!searchPackage(db, name, entry)) {
        errLine("pkg: package '%s' not found", name.c_str());
        return false;
    }

    if (!entry.installed) {
        printLine("pkg: package '%s' is not installed", name.c_str());
        return true;
    }

    std::string installedPath = joinPath("/bin", entry.name);
    if (unlink(installedPath.c_str()) < 0) {
        if (errno != ENOENT) {
            errLine("pkg: cannot remove '%s': %s", installedPath.c_str(), strerror(errno));
            return false;
        }
        // Already gone from /bin; still clear the flag so the db matches.
        errLine("pkg: '%s' was already missing", installedPath.c_str());
    }

    std::string tarballPath = joinPath(joinPath(repo, "packages"), entry.name + "-" + versionString(entry.version) + ".tar");
    if (!stripInstallFlag(tarballPath)) {
        errLine("pkg: '%s' was removed from /bin but is still marked installed", name.c_str());
        return false;
    }

    // Rebuild installed/size state from the tarballs.
    if (!loadDB(db)) {
        errLine("pkg: '%s' removed, but reloading the database failed", name.c_str());
        return false;
    }

    printLine("pkg: package '%s' removed successfully", name.c_str());
    return true;
}

bool run(const Args &args) {
    std::vector<dbEntry> db;
    if (!loadDB(db)) {
        errLine("pkg: failed to load package database");
        return false;
    }

    // Indexed by Command; order must match the enum.
    using Handler = bool (*)(std::vector<dbEntry> &, const std::string &);
    static const Handler table[] = {
        [](std::vector<dbEntry> &d, const std::string &)   { return cmdList(d); },
        [](std::vector<dbEntry> &d, const std::string &n)  { return cmdInfo(d, n); },
        [](std::vector<dbEntry> &d, const std::string &n)  { return cmdInstall(d, n); },
        [](std::vector<dbEntry> &d, const std::string &n)  { return cmdRemove(d, n); },
    };

    size_t i = static_cast<size_t>(args.command);
    if (i >= sizeof(table) / sizeof(table[0])) {
        usage();
        return false;
    }
    
    return table[i](db, args.name);
}

int main(int argc, char **argv) {
    Args args;
    if (!parseArgs(argc, argv, args)) {
        usage();
        return 1;
    }

    return run(args) ? 0 : 1;
}
