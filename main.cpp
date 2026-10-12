#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>

#include "./src/include/echo.h"

static void run_command(const std::vector<std::string>& args) {}

int main()
{
    std::string input;

    while(std::cout << "$\x20\x20" && std::getline(std::cin, input)){
        if (input == "exit"){
            std::cout << "Bye" << std::endl;
            break;
        }
    }

    return 0;
}

