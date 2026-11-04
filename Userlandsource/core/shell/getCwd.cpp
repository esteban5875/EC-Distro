#include <unistd.h>  
#include <cerrno>
#include <cstring>   

#include "../header/builtInCommands.h"
#include "../header/constants.h"

int getWorkingDir(){
    int status = 1;

    if (getcwd(SHELL_PRINT_BUFFER, sizeof(SHELL_PRINT_BUFFER)) == nullptr) {
        errLine("pwd: %s", std::strerror(errno));
        std::memset(SHELL_PRINT_BUFFER, 0, sizeof(SHELL_PRINT_BUFFER));
        status = 0;
    }
    
    return status;
}

int builtInPwd(){
    char buf[4096];

    if (getcwd(buf, sizeof(buf)) == nullptr) {
        errLine("pwd: %s", std::strerror(errno));
        return 1;
    }

    printLine("%s", buf);
    return 0;
}