#include <unistd.h>
#include <cerrno>
#include <cstring>
#include "../header/builtInCommands.h"
#include "../header/constants.h"

int builtInChdir(Command* cmd) {
    if (cmd->args.size() > 1) {
        errLine("cd: too many arguments");
        return 1;
    }
    
    if (cmd->args.empty()) {
        if (chdir("/") != 0) {
            errLine("cd: /: %s", std::strerror(errno));
            return 1;
        }
        return 0;
    }

    const std::string& target = cmd->args[0];
    if (chdir(target.c_str()) != 0) {
        errLine("cd: %s: %s", target.c_str(), std::strerror(errno));
        return 1;
    }
    
    return 0;
}