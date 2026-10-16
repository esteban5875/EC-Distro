#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <iostream>

inline char SHELL_PRINT_BUFFER[4096];

inline bool vprintLine(std::ostream& out, const char* fmt, va_list ap) {
    const std::size_t cap = sizeof(SHELL_PRINT_BUFFER) - 1;
    int n = std::vsnprintf(SHELL_PRINT_BUFFER, cap, fmt, ap);

    if (n < 0 || static_cast<std::size_t>(n) >= cap) {
        std::cerr << "message too big for shell buffer\n";
        return false;
    }

    SHELL_PRINT_BUFFER[n] = '\n';
    out.write(SHELL_PRINT_BUFFER, n + 1);
    return true;
}

__attribute__((format(printf, 1, 2)))
inline bool printLine(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    bool ok = vprintLine(std::cout, fmt, ap);
    va_end(ap);
    return ok;
}

__attribute__((format(printf, 1, 2)))
inline bool errLine(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    bool ok = vprintLine(std::cerr, fmt, ap);
    va_end(ap);
    return ok;
}
