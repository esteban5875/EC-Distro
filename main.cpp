#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

//Tools

//Shell Logic
#include "./src/include.h"

static inline int prompt(){
    std::cout << "$\x20\x20";
    return 1;
}

static void run_proc(const std::vector<std::string>& args) {}

int main()
{
    setenv("PATH", "/bin", 0);

    std::string input;

    while(prompt() && std::getline(std::cin, input)){
        if (input == "exit"){
            prompt();
            std::cout << "Bye" << std::endl;
            break;
        }

        Command outCommand;

        int parseCommand = parse(input, &outCommand);

        if (parseCommand == 1){
            continue;
        }

        switch (outCommand.name)
        {
        case CommandName::Echo:
            echo(&outCommand);
            break;
        
        case CommandName::Unknown:
            std::cout << "Unknown command:" + outCommand.enteredInput << std::endl;
            break;
        }
    }

    return 0;
}

