#include "./header/pkg.h"

#include <cerrno>
#include <fstream>
#include <iterator>
#include <sys/stat.h>

std::string joinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (a.back() == '/') return a + b;
    return a + "/" + b;
}

std::vector<std::string> splitPath(const std::string& s) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : s) {
        if (c == '/') {
            if (!cur.empty()) parts.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) parts.push_back(cur);
    return parts;
}

bool mkdirP(const std::string& absPath) {
    std::string cur = (!absPath.empty() && absPath[0] == '/') ? "/" : "";
    for (const std::string& comp : splitPath(absPath)) {
        cur = joinPath(cur, comp);
        if (mkdir(cur.c_str(), 0755) != 0 && errno != EEXIST) return false;
    }
    return true;
}

// Strips leading "./" and trailing "/". "./" and "." become "".
std::string normalizeRel(std::string n) {
    while (n.rfind("./", 0) == 0) n.erase(0, 2);
    while (!n.empty() && n.back() == '/') n.pop_back();
    if (n == ".") n.clear();
    return n;
}

// A path from an archive or record must stay inside the root.
bool isSafeRel(const std::string& n) {
    if (n.empty() || n[0] == '/') return false;
    for (const std::string& c : splitPath(n))
        if (c == "..") return false;
    return true;
}

bool validName(const std::string& n) {
    return !n.empty() && n[0] != '.' && n.find('/') == std::string::npos;
}

bool readFile(const std::string& path, std::vector<char>& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}