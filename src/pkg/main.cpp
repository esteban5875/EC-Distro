#include "./header/pkg.h"
#include "../tools/common/constants.h"

static int usage() {
    errLine("usage: pkg [--root DIR] [--repo DIR] <list|info|install|remove> [name]");
    return 2;
}

static int cmdList(const Config& cfg) {
    std::vector<IndexEntry> idx;
    if (!loadIndex(cfg, idx)) return 1;

    for (const IndexEntry& e : idx)
        printLine("%s %s%s", e.name.c_str(), e.version.c_str(),
                  isInstalled(cfg, e.name) ? " [installed]" : "");
    return 0;
}

static int cmdInfo(const Config& cfg, const std::string& name) {
    std::vector<IndexEntry> idx;
    if (!loadIndex(cfg, idx)) return 1;

    const IndexEntry* e = findEntry(idx, name);
    Record rec;
    bool installed = loadRecord(cfg, name, rec);

    if (!e && !installed) {
        errLine("pkg: %s: not found", name.c_str());
        return 1;
    }

    printLine("name: %s", name.c_str());
    if (e) printLine("available: %s (%s)", e->version.c_str(), e->filename.c_str());
    if (installed) {
        printLine("installed: %s", rec.version.c_str());
        for (const auto& f : rec.files) printLine("  /%s", f.c_str());
    } else {
        printLine("installed: no");
    }
    return 0;
}

static int cmdInstall(const Config& cfg, const std::string& name) {
    if (isInstalled(cfg, name)) {
        errLine("pkg: %s: already installed", name.c_str());
        return 1;
    }

    std::vector<IndexEntry> idx;
    if (!loadIndex(cfg, idx)) return 1;

    const IndexEntry* e = findEntry(idx, name);
    if (!e) {
        errLine("pkg: %s: not in the repo", name.c_str());
        return 1;
    }

    std::string tarPath;
    if (!fetchPackage(cfg, *e, tarPath)) return 1;

    std::vector<char> data;
    if (!readFile(tarPath, data)) {
        errLine("pkg: cannot read %s", tarPath.c_str());
        return 1;
    }

    // TODO (later): verify sha256 of `data` against e->sha256 before extracting.

    Record rec;
    rec.version = e->version;

    if (!extractTar(cfg, data, rec) || !saveRecord(cfg, name, rec)) {
        removeRecordFiles(cfg, rec);
        errLine("pkg: %s: install failed", name.c_str());
        return 1;
    }

    printLine("installed %s %s (%zu files)", name.c_str(), rec.version.c_str(), rec.files.size());
    return 0;
}

static int cmdRemove(const Config& cfg, const std::string& name) {
    Record rec;
    if (!loadRecord(cfg, name, rec)) {
        errLine("pkg: %s: not installed", name.c_str());
        return 1;
    }

    removeRecordFiles(cfg, rec);
    deleteRecord(cfg, name);
    printLine("removed %s", name.c_str());
    return 0;
}

int main(int argc, char* argv[]) {
    Config cfg;
    std::vector<std::string> pos;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--root" && i + 1 < argc)      cfg.root = argv[++i];
        else if (a == "--repo" && i + 1 < argc) cfg.repo = argv[++i];
        else                                    pos.push_back(a);
    }
    if (pos.empty()) return usage();

    const std::string& cmd = pos[0];
    if (cmd == "list") return cmdList(cfg);

    if (pos.size() != 2) return usage();
    const std::string& name = pos[1];
    if (!validName(name)) {
        errLine("pkg: invalid package name: %s", name.c_str());
        return 1;
    }

    if (cmd == "info")    return cmdInfo(cfg, name);
    if (cmd == "install") return cmdInstall(cfg, name);
    if (cmd == "remove")  return cmdRemove(cfg, name);
    return usage();
}