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

//TODO: Fetch tarball

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

int main(int argc, char **argv) {}
