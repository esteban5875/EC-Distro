#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

#include "./src/include.h"

static void run_proc(const std::vector<std::string>& args) {}

int main()
{
    setenv("PATH", "/bin", 0);

    std::string input;
    int lastStatus = 0;

    while(prompt() && std::getline(std::cin, input)){
        Command outCommand;

        int parseCommand = parse(input, &outCommand);

        if (parseCommand == 1){
            continue;
        }

        if (outCommand.name == CommandName::Exit){
            prompt();
            printLine("Bye");
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
