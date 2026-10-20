#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

#include "./src/include.h"

int main()
{
    setenv("PATH", "/bin", 0);
    const bool isInit = (getpid() == 1);
    if (isInit) initSystem();

    std::string input;
    int lastStatus = 0;

    while(true){
        if (isInit) readOrphans();
        prompt(); 

        if (!std::getline(std::cin, input)) {       
            printLine("exit");
            if (isInit) powerOff();
            break;
        }

        Command outCommand{};
        if (parse(input, &outCommand) == 1) continue;

        if (outCommand.name == CommandName::Exit) {
            printLine("Bye");
            if (isInit) powerOff();
            break;
        }

        switch (outCommand.name)
        {
            case CommandName::Exit:
                break;
            case CommandName::Clear:
                clearScreen();
                lastStatus = 0;
                break;
            case CommandName::Echo:
                lastStatus = builtInEcho(&outCommand); 
                break;
            case CommandName::Type:
                lastStatus = builtInType(&outCommand);
                break;
            case CommandName::Pwd:
                lastStatus = builtInPwd();
                break;
            case CommandName::Cd:
                lastStatus = builtInChdir(&outCommand);
                break;
            case CommandName::External: {
                LookupResult r = findInPath(outCommand.enteredInput);

                switch (r.status) {
                    case LookupStatus::Found:
                        lastStatus = runExternal(r.path, &outCommand);
                        break;
                    case LookupStatus::NotFound:
                    case LookupStatus::PathUnset:
                        errLine("%s: command not found", outCommand.enteredInput.c_str());
                        lastStatus = 127;
                        break;
                    case LookupStatus::NotExecutable:
                    case LookupStatus::NotRegular:
                        errLine("%s: cannot execute", outCommand.enteredInput.c_str());
                        lastStatus = 126;
                        break;
                }

                break;
            }
        }
    }
}
