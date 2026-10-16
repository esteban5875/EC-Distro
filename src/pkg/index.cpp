#include "./header/pkg.h"
#include "../tools/common/constants.h"

#include <fstream>
#include <sstream>
#include <sys/stat.h>

bool loadIndex(const Config& cfg, std::vector<IndexEntry>& out) {
    std::string path = joinPath(cfg.repo, "index");
    std::ifstream in(path);
    if (!in) {
        errLine("pkg: cannot open %s", path.c_str());
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ss(line);
        IndexEntry e;
        if (!(ss >> e.name >> e.version >> e.filename)) continue;
        ss >> e.sha256;
        out.push_back(e);
    }
    return true;
}

const IndexEntry* findEntry(const std::vector<IndexEntry>& idx, const std::string& name) {
    for (const IndexEntry& e : idx)
        if (e.name == name) return &e;
    return nullptr;
}

bool fetchPackage(const Config& cfg, const IndexEntry& e, std::string& localPath) {
    if (!isSafeRel(e.filename)) {
        errLine("pkg: %s: unsafe filename in index", e.name.c_str());
        return false;
    }
    localPath = joinPath(cfg.repo, "packages/" + e.filename);

    struct stat st;
    if (stat(localPath.c_str(), &st) != 0) {
        errLine("pkg: %s: missing %s", e.name.c_str(), localPath.c_str());
        return false;
    }
    return true;
}