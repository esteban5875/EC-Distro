#pragma once

#include <unistd.h>
#include <cstring>

#include "./header/shell.h"
#include "./header/commandFunc.h"
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
}

//Void Return Commands (Direct Action)

EXPOSE_COMMAND_VOID(echo);

//Int Return Commands (Error Handling)

EXPOSE_COMMAND_INT(builtInType);
EXPOSE_COMMAND_INT(builtInChdir);
EXPOSE_COMMAND_INT(builtInPwd);