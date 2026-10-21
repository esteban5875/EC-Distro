#pragma once

#include "../../tools/tools-common/constants.h"

inline constexpr const char* root = "/";
inline constexpr const char* repo = "/var/repo";

// Appended to the end of a package tarball when it is installed, and
// truncated off when it is removed. Lives past the tar's end-of-archive
// blocks, so tar readers never see it as an entry.
inline constexpr char   INSTALL_FLAG[]   = "ECPKG:INSTALLED\n";
inline constexpr size_t INSTALL_FLAG_LEN = sizeof(INSTALL_FLAG) - 1;   // without the NUL

inline std::string joinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    if (a.back() == '/') return a + b;
    return a + "/" + b;
}