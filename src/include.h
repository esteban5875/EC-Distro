#pragma once

#include <unistd.h>
#include <cstring>

#include "./header/shell.h"
#include "./header/builtInCommands.h"
#include "./header/constants.h"


//Simple prompt func

inline int prompt(){
    int status = getWorkingDir();
    if (status == 1){
        std::cout << "$\x20" << SHELL_PRINT_BUFFER << "\x20\x20:" << "\x20\x20";
        std::memset(SHELL_PRINT_BUFFER, 0, sizeof(SHELL_PRINT_BUFFER));
    } else {
        std::cout << "$\x20\x20";
    }

    return status;
}

//Simple clear func

inline void clearScreen() {
    static constexpr char seq[] = "\x1b[H\x1b[2J\x1b[3J";
    (void)!write(STDOUT_FILENO, seq, sizeof(seq) - 1);
    std::memset(SHELL_PRINT_BUFFER, 0, sizeof(SHELL_PRINT_BUFFER));
}