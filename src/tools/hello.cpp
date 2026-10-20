#include "./tools-common/constants.h"

//Test tool

int main(int argc, char* const argv[])
{
    (void)argv;        

    if (argc > 1){
        errLine("hello: bad input");
        return 1;
    }

    printLine("Hello command success");
    return 0;
}
