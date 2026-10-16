#include <unistd.h>  
#include <cerrno>
#include <cstring>   
#include <iostream>

#include "../header/commandFunc.h"
#include "../header/constants.h"

int getWorkingDir(){
    int status = 1;

    if (getcwd(SHELL_PRINT_BUFFER, sizeof(SHELL_PRINT_BUFFER)) == nullptr) {
        std::cerr << "pwd: " << std::strerror(errno) << '\n';
        std::memset(SHELL_PRINT_BUFFER, 0, sizeof(SHELL_PRINT_BUFFER));
        status = 0;
    }
    
    return status;
}

DEFINE_COMMAND_INT_RETURN(builtInPwd){
    char buf[4096];

    int pwdStatus = getWorkingDir();

    if (pwdStatus == 1){
        std::cout << SHELL_PRINT_BUFFER << '\n';
    } else {
        std::cerr << "pwd: " << std::strerror(errno) << '\n';
    }

    return pwdStatus;
}