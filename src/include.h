#pragma once

#include <unistd.h>

#include "./header/shell.h"
#include "./header/commandFunc.h"


//Simple prompt func

inline int prompt(){
    std::cout << "$\x20\x20";
    return 1;
}

//Simple clear func

inline void clearScreen() {
    static constexpr char seq[] = "\x1b[H\x1b[2J\x1b[3J";
    (void)!write(STDOUT_FILENO, seq, sizeof(seq) - 1);
}

//Void Return Commands (Direct Action)

EXPOSE_COMMAND_VOID(echo);

//Int Return Commands (Error Handling)

EXPOSE_COMMAND_INT(builtInType);