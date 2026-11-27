#include <stdio.h>

#include "../header/cmd/echo.h"

#include "../header/usage.h"

int echo(const char** args, const size_t args_num, const size_t max_args){
    if (args == NULL) {
        perror("Received a NULL pointer for echo");
        return 0;
    }

    if (args_num > max_args){
        perror("Error: Too many arguments");
        return 0;
    }  

    if (strcmp(args[0], "-h")){
        printf("%s\n", ECHO_USAGE);
        return 1;
    } 
    
    else {
        printf("%s", args[0]);
        return 1;
    }
}