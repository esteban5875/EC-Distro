#include <iostream>

#include "../header/commandFunc.h"
#include "../header/typeData.h"
#include "../header/shell.h"

DEFINE_COMMAND_INT_RETURN(builtInType) {
    int status = 0;

    if (cmd->args.size() == 0){
        return status;
    }

    for (const std::string& name : cmd->args) {
        if (CommandMap.find(name) != CommandMap.end()) {
            std::cout << name << " is a shell builtin\n";
            continue;
        }

        LookupResult r = findInPath(name);

        switch (r.status) {
            case LookupStatus::Found:
                std::cout << name << " is " << r.path << '\n';
                break;
            case LookupStatus::NotFound:
                std::cerr << name << ": not found\n";
                status = 1;
                break;
            case LookupStatus::NotExecutable:
                std::cerr << name << ": " << r.path << ": permission denied\n";
                status = 1;
                break;
            case LookupStatus::NotRegular:
                std::cerr << name << ": not a regular file\n";
                status = 1;
                break;
            case LookupStatus::PathUnset:
                std::cerr << name << ": PATH is not set\n";
                status = 1;
                return status;
        }
    }

    return status;
}
