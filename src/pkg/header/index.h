#pragma once

#include <string>
#include <vector>

struct indexEntry {
    std::string name;
    float       version;
    std::string filename;
    std::string SHA256;
};

int loadIndex(std::vector<indexEntry> &out);