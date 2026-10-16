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
        if (input == "exit"){
            prompt();
            std::snprintf(SHELL_PRINT_BUFFER, sizeof(SHELL_PRINT_BUFFER), "%s", "Bye");
            std::cout << SHELL_PRINT_BUFFER << '\n';
            std::memset(SHELL_PRINT_BUFFER, 0, sizeof(SHELL_PRINT_BUFFER));
            break;
        } else if (input == "clear") {
            clearScreen();
            continue;
        }

        Command outCommand;

        int parseCommand = parse(input, &outCommand);

        if (parseCommand == 1){
            continue;
        }

        switch (outCommand.name)
        {
            case CommandName::Echo:
                lastStatus = builtInEcho(&outCommand, SHELL_PRINT_BUFFER); 
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
                        std::cerr << outCommand.enteredInput << ": command not found\n";
                        lastStatus = 127;
                        break;
                    case LookupStatus::NotExecutable:
                    case LookupStatus::NotRegular:
                        std::cerr << outCommand.enteredInput << ": cannot execute\n";
                        lastStatus = 126;
                        break;
                }

                break;
            }
        }
    }
}
