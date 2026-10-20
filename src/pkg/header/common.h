#pragma once

#include "../../tools/tools-common/constants.h"

inline constexpr char* root = "/";
inline constexpr char* repo = "/var/repo";

inline std::string joinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    if (a.back() == '/') return a + b;
    return a + "/" + b;
}