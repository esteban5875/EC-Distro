#include "./header/pkg.h"
#include "../tools/common/constants.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unistd.h>

static std::string dbDir(const Config& c) { return joinPath(c.root, "var/lib/pkg"); }
static std::string dbPath(const Config& c, const std::string& n) { return joinPath(dbDir(c), n); }

bool isInstalled(const Config& cfg, const std::string& name) {
    return access(dbPath(cfg, name).c_str(), F_OK) == 0;
}

bool saveRecord(const Config& cfg, const std::string& name, const Record& rec) {
    if (!mkdirP(dbDir(cfg))) {
        errLine("pkg: cannot create %s: %s", dbDir(cfg).c_str(), std::strerror(errno));
        return false;
    }

    std::string path = dbPath(cfg, name);
    std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp);
        out << "version " << rec.version << '\n';
        for (const auto& d : rec.dirs)  out << "d " << d << '\n';
        for (const auto& f : rec.files) out << "f " << f << '\n';
        out.flush();
        if (!out) {
            errLine("pkg: cannot write %s", tmp.c_str());
            return false;
        }
    }
    if (rename(tmp.c_str(), path.c_str()) != 0) {
        errLine("pkg: %s", std::strerror(errno));
        unlink(tmp.c_str());
        return false;
    }
    return true;
}

bool loadRecord(const Config& cfg, const std::string& name, Record& rec) {
    std::ifstream in(dbPath(cfg, name));
    if (!in) return false;

    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("version ", 0) == 0) {
            rec.version = line.substr(8);
        } else if (line.size() > 2 && (line[0] == 'f' || line[0] == 'd') && line[1] == ' ') {
            std::string p = line.substr(2);
            if (!isSafeRel(p)) {
                errLine("pkg: ignoring unsafe path in record: %s", p.c_str());
                continue;
            }
            (line[0] == 'f' ? rec.files : rec.dirs).push_back(p);
        }
    }
    return true;
}

void removeRecordFiles(const Config& cfg, const Record& rec) {
    for (const std::string& f : rec.files) {
        std::string abs = joinPath(cfg.root, f);
        if (unlink(abs.c_str()) != 0 && errno != ENOENT)
            errLine("pkg: cannot remove %s: %s", abs.c_str(), std::strerror(errno));
    }
    for (auto it = rec.dirs.rbegin(); it != rec.dirs.rend(); ++it) {
        std::string abs = joinPath(cfg.root, *it);
        if (rmdir(abs.c_str()) != 0 && errno != ENOTEMPTY && errno != ENOENT)
            errLine("pkg: cannot remove %s: %s", abs.c_str(), std::strerror(errno));
    }
}

bool deleteRecord(const Config& cfg, const std::string& name) {
    return unlink(dbPath(cfg, name).c_str()) == 0;
}