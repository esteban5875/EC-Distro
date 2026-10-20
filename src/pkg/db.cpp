#include "./header/db.h"
#include "./header/index.h"
#include "./header/common.h"

#include <dirent.h>
#include <cstdio>

static std::string versionToString(float v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%g", v);     // 1.0 -> "1", 1.5 -> "1.5", 1.25 -> "1.25"
    std::string s = buf;
    if (s.find('.') == std::string::npos) s += ".0";   // "1" -> "1.0"
    return s;
}

bool loadDB(std::vector<dbEntry> &out) {
    std::vector<indexEntry> index;
    if (loadIndex(index) < 0) {
        return false;
    }

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

    const std::string packagesPath = joinPath(repo, "packages");

    DIR *dir = opendir(packagesPath.c_str());
    if (!dir) {
        errLine("pkg: cannot open %s", packagesPath.c_str());
        return false;
    }

    struct dirent *de;

    // Search the packages directory for .tar files and update the installed
    // status and size in the dbEntry vector
    while ((de = readdir(dir)) != nullptr) {
        if (de->d_type != DT_REG && de->d_type != DT_UNKNOWN) continue;   // regular files only

        std::string filename(de->d_name);
        if (filename.size() <= 4 || filename.substr(filename.size() - 4) != ".tar") continue;

        std::string tarName = filename.substr(0, filename.size() - 4);

        for (auto &item : out) {
            if (tarName != item.name + "-" + versionToString(item.version)) continue;

            item.installed = true;
            std::string packagePath = joinPath(packagesPath, filename);

            FILE *file = fopen(packagePath.c_str(), "rb");
            if (!file) {
                errLine("pkg: cannot open %s", packagePath.c_str());
                closedir(dir);
                return false;
            }

            fseek(file, 0, SEEK_END);
            long bytes = ftell(file);
            fclose(file);

            if (bytes >= 0) item.size = static_cast<float>(bytes);
            break;                 // found the package, stop comparing
        }
    }

    closedir(dir);
    return true;
}