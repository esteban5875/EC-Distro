#include "header/pkg.h"                     
#include "../tools/common/constants.h"
      
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static std::size_t parseOctal(const char* p, std::size_t n) {
    std::size_t v = 0, i = 0;
    while (i < n && p[i] == ' ') ++i;
    for (; i < n && p[i] >= '0' && p[i] <= '7'; ++i) v = v * 8 + static_cast<std::size_t>(p[i] - '0');
    return v;
}

static bool isZeroBlock(const char* h) {
    for (int i = 0; i < 512; ++i)
        if (h[i] != 0) return false;
    return true;
}

// Creates every directory in relDir under root, recording the ones that were new.
static bool ensureDirs(const Config& cfg, const std::string& relDir, Record& rec) {
    std::string prefix;
    for (const std::string& c : splitPath(relDir)) {
        prefix = prefix.empty() ? c : prefix + "/" + c;
        std::string abs = joinPath(cfg.root, prefix);
        if (mkdir(abs.c_str(), 0755) == 0) {
            rec.dirs.push_back(prefix);
        } else if (errno != EEXIST) {
            errLine("pkg: mkdir %s: %s", abs.c_str(), std::strerror(errno));
            return false;
        }
    }
    return true;
}

// Writes to a temporary name and renames it, so a crash never leaves a half-written binary.
static bool writeFile(const std::string& abs, const char* data, std::size_t size, mode_t mode) {
    std::string tmp = abs + ".pkgtmp";
    int fd = open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        errLine("pkg: open %s: %s", tmp.c_str(), std::strerror(errno));
        return false;
    }

    std::size_t off = 0;
    while (off < size) {
        ssize_t n = write(fd, data + off, size - off);
        if (n < 0) {
            if (errno == EINTR) continue;
            errLine("pkg: write %s: %s", tmp.c_str(), std::strerror(errno));
            close(fd);
            unlink(tmp.c_str());
            return false;
        }
        off += static_cast<std::size_t>(n);
    }

    // Mode bits from the archive. Without the execute bit, a tool is "not executable".
    if (fchmod(fd, mode) != 0) {
        errLine("pkg: chmod %s: %s", tmp.c_str(), std::strerror(errno));
        close(fd);
        unlink(tmp.c_str());
        return false;
    }
    close(fd);

    if (rename(tmp.c_str(), abs.c_str()) != 0) {
        errLine("pkg: rename %s: %s", abs.c_str(), std::strerror(errno));
        unlink(tmp.c_str());
        return false;
    }
    return true;
}

bool extractTar(const Config& cfg, const std::vector<char>& data, Record& rec) {
    std::size_t pos = 0;

    while (pos + 512 <= data.size()) {
        const char* h = data.data() + pos;
        if (isZeroBlock(h)) break;                       // end of archive

        if (std::memcmp(h + 257, "ustar", 5) != 0) {
            errLine("pkg: not a ustar archive");
            return false;
        }

        std::string name(h, strnlen(h, 100));
        std::string prefix(h + 345, strnlen(h + 345, 155));
        if (!prefix.empty()) name = prefix + "/" + name;

        mode_t mode = static_cast<mode_t>(parseOctal(h + 100, 8) & 07777);
        std::size_t size = parseOctal(h + 124, 12);
        char type = h[156];

        pos += 512;
        if (pos + size > data.size()) {
            errLine("pkg: truncated archive");
            return false;
        }
        const char* body = data.data() + pos;
        pos += (size + 511) / 512 * 512;

        std::string rel = normalizeRel(name);
        if (rel.empty()) continue;                       // the "./" entry
        if (!isSafeRel(rel)) {
            errLine("pkg: unsafe path in archive: %s", name.c_str());
            return false;
        }

        if (type == '5') {                               // directory
            if (!ensureDirs(cfg, rel, rec)) return false;
        } else if (type == '0' || type == '\0') {        // regular file
            std::string::size_type slash = rel.rfind('/');
            if (slash != std::string::npos && !ensureDirs(cfg, rel.substr(0, slash), rec))
                return false;

            std::string abs = joinPath(cfg.root, rel);
            struct stat st;
            if (lstat(abs.c_str(), &st) == 0) {          // never overwrite a file we don't own
                errLine("pkg: conflict: %s already exists", abs.c_str());
                return false;
            }
            if (!writeFile(abs, body, size, mode)) return false;
            rec.files.push_back(rel);
        } else {
            errLine("pkg: unsupported entry type '%c' for %s", type, rel.c_str());
            return false;
        }
    }
    return true;
}