#include "./header/index.h"
#include "./header/common.h"

#include <fstream>
#include <sstream>

static std::string joinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    if (a.back() == '/') return a + b;
    return a + "/" + b;
}

int loadIndex(std::vector<indexEntry> &out){
    std::string path = joinPath(repo, "index");
    std::ifstream in(path);

    if (!in){
        errLine("pkg: cannot open %s", path.c_str());
        return false;
    }

    std::string line;
    while(std::getline(in, line)){
        std::istringstream ss(line);
        indexEntry e;
        if (!(ss >> e.name >> e.version >> e.filename)) continue;
        ss >> e.SHA256;
        out.push_back(e);
    }

    return true;
}