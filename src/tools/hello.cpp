#include "./common/constants.h"

int main(int argc, char* argv[])
{
    (void)argv;        

    if (argc > 1){
        errLine("hello: bad input");
        return 1;
    }

    printLine("Hello command success");
    return 0;
}
