#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>

#include "../header/commandFunc.h"

DEFINE_COMMAND_INT_RETURN(builtInChdir) {
    if (cmd->args.size() > 1) {
        std::cerr << "cd: too many arguments\n";
        return 1;
    }
    if (cmd->args.empty()) {
        return 0;   // first version: no HOME support
    }

    const std::string& target = cmd->args[0];
    if (chdir(target.c_str()) != 0) {
        std::cerr << "cd: " << target << ": " << std::strerror(errno) << '\n';
        return 1;
    }
    return 0;
}