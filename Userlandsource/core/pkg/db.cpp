#include "./header/db.h"
#include "./header/index.h"
#include "./header/common.h"

#include <dirent.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

static std::string versionToString(float v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%g", v);     // 1.0 -> "1", 1.5 -> "1.5", 1.25 -> "1.25"
    std::string s = buf;
    if (s.find('.') == std::string::npos) s += ".0";   // "1" -> "1.0"
    return s;
}

static void initializeDBEntry(const std::vector<indexEntry> &index, std::vector<dbEntry> &out) {
    for (size_t i = 0; i < index.size(); ++i) {
        const auto &entry = index[i];
        dbEntry item;
        item.id = static_cast<int>(i);
        item.name = entry.name;
        item.version = entry.version;
        item.size = 0.0f;          // Placeholder for size, as it's not available in index
        item.installed = false;    // Placeholder for installed status, as it's not available in index
        out.push_back(item);
    }
}

// True if the tarball's last INSTALL_FLAG_LEN bytes are INSTALL_FLAG.
static bool hasInstallFlag(FILE *file, long bytes) {
    if (bytes < static_cast<long>(INSTALL_FLAG_LEN)) return false;
    if (fseek(file, bytes - static_cast<long>(INSTALL_FLAG_LEN), SEEK_SET) != 0) return false;

    char tail[INSTALL_FLAG_LEN];
    if (fread(tail, 1, INSTALL_FLAG_LEN, file) != INSTALL_FLAG_LEN) return false;
    return memcmp(tail, INSTALL_FLAG, INSTALL_FLAG_LEN) == 0;
}

static void updateDBEntry(dbEntry &item, const std::string &filename) {
    std::string tarName = filename.substr(0, filename.size() - 4); // Remove ".tar"
    if (tarName == item.name + "-" + versionToString(item.version)) {
        std::string packagePath = joinPath(joinPath(repo, "packages"), filename);

        FILE *file = fopen(packagePath.c_str(), "rb");
        if (!file) {
            errLine("pkg: cannot open %s", packagePath.c_str());
            return;
        }

        fseek(file, 0, SEEK_END);
        long bytes = ftell(file);
        item.installed = bytes >= 0 && hasInstallFlag(file, bytes);
        fclose(file);

        // Report the package's size, not including the flag.
        if (item.installed) bytes -= static_cast<long>(INSTALL_FLAG_LEN);
        if (bytes >= 0) item.size = static_cast<float>(bytes);
    }
}

// Writes a lookup file with one "ID:Name" pair per line, so a package's id
// can be found by name without scanning the full db. The file is only
// rewritten when its contents change, so read-only use works without root.
static bool writeNameDB(const std::vector<dbEntry> &db) {
    const std::string path = joinPath(repo, "namedb");

    std::ostringstream content;
    for (const auto &item : db) {
        content << item.id << ':' << item.name << '\n';
    }

    std::ifstream existing(path);
    if (existing) {
        std::ostringstream current;
        current << existing.rdbuf();
        if (current.str() == content.str()) return true;
    }

    std::ofstream file(path, std::ios::trunc);
    if (!file) {
        errLine("pkg: cannot write %s", path.c_str());
        return false;
    }

    file << content.str();
    return static_cast<bool>(file);
}

// Binary search by id. Works because ids are assigned in increasing order
// when the db is built, so `db` is always sorted by id.
static bool searchPackageById(const std::vector<dbEntry> &db, int id, dbEntry &out) {
    size_t low = 0;
    size_t high = db.size();          

    while (low < high) {
        size_t mid = low + (high - low) / 2;

        if (db[mid].id == id) {
            out = db[mid];
            return true;
        }

        if (db[mid].id < id) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    return false;
}


bool loadDB(std::vector<dbEntry> &out) {
    out.clear();

    std::vector<indexEntry> index;
    if (!loadIndex(index)) {
        return false;
    }

    initializeDBEntry(index, out);

    const std::string packagesPath = joinPath(repo, "packages");

    DIR *dir = opendir(packagesPath.c_str());
    if (!dir) {
        errLine("pkg: cannot open %s", packagesPath.c_str());
        return false;
    }

    struct dirent *de;

    // Search the packages directory for .tar files and update the installed
    // status (install flag at the end of the tarball) and size
    while ((de = readdir(dir)) != nullptr) {
        std::string filename = de->d_name;
        if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".tar") {
            for (auto &item : out) {
                updateDBEntry(item, filename);
            }
        }
    }

    closedir(dir);

    // A stale or unwritable namedb only affects name lookups, so it
    // shouldn't fail the whole load.
    writeNameDB(out);
    return true;
}


// Looks up the package id by name in the namedb file, then binary searches
// the db for the full entry.
bool searchPackage(const std::vector<dbEntry> &db, const std::string &name, dbEntry &out) {
    const std::string path = joinPath(repo, "namedb");
    std::ifstream file(path);
    if (!file) {
        errLine("pkg: cannot open %s", path.c_str());
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t sep = line.find(':');
        if (sep == std::string::npos) continue;
        if (line.compare(sep + 1, std::string::npos, name) != 0) continue;

        try {
            int id = std::stoi(line.substr(0, sep));
            return searchPackageById(db, id, out);
        } catch (const std::exception &) {
            return false;
        }
    }

    return false;
}
