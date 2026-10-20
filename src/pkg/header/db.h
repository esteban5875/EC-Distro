#pragma once

#include <string>
#include <vector>

struct dbEntry {
    int id;
    std::string name;
    float       version;
    float      size;
    bool       installed;
};

bool loadDB(std::vector<dbEntry> &out);
bool searchPackage(const std::string &name, dbEntry &out);
bool updatePkg(const std::string &name);
