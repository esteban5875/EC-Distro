#include <iostream>
#include <cstring>

#include "./common/constants.h"

int main(int argc, char* argv[])
{
    std::snprintf(
        CHILD_SHELL_PRINT_BUFFER, 
        sizeof(CHILD_SHELL_PRINT_BUFFER), 
        "Hello external command success."
    );
    std::cout << CHILD_SHELL_PRINT_BUFFER << '\n';
    std::memset(CHILD_SHELL_PRINT_BUFFER, 0, sizeof(CHILD_SHELL_PRINT_BUFFER));
    return 0;
}
