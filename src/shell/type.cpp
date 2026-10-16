#include "../header/builtInCommands.h"
#include "../header/constants.h"
#include "../header/typeData.h"
#include "../header/shell.h"

int builtInType(Command* cmd) {
    int status = 0;

    if (cmd->args.size() == 0){
        return status;
    }

    for (const std::string& name : cmd->args) {
        if (CommandMap.find(name) != CommandMap.end()) {
            printLine("%s is a shell builtin", name.c_str());
            continue;
        }

        LookupResult r = findInPath(name);

        switch (r.status) {
            case LookupStatus::Found:
                printLine("%s is %s", name.c_str(), r.path.c_str());
                break;
            case LookupStatus::NotFound:
                errLine("%s: not found", name.c_str());
                status = 1;
                break;
            case LookupStatus::NotExecutable:
                errLine("%s: %s: permission denied", name.c_str(), r.path.c_str());
                status = 1;
                break;
            case LookupStatus::NotRegular:
                errLine("%s: not a regular file", name.c_str());
                status = 1;
                break;
            case LookupStatus::PathUnset:
                errLine("%s: PATH is not set", name.c_str());
                status = 1;
                return status;
        }
    }

    return status;
}
