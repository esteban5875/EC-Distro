#pragma once

#include <string>
#include <vector>

struct indexEntry {
    std::string name;
    float       version;
    std::string filename;
};

bool loadIndex(std::vector<indexEntry> &out);