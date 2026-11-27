#pragma once

#include <string.h>

#include "shell.h"

#include "cmd/echo.h"


inline LOOKUP_COMMAND SHELL_LOOKUP_TABLE[] = {
    {ECHO, "echo", 1, echo}
};

inline int table_lookup(const char* cmd){
    for (size_t i = 0; i < sizeof(SHELL_LOOKUP_TABLE) / sizeof(SHELL_LOOKUP_TABLE[0]); i++)
    {
        if (strcmp(SHELL_LOOKUP_TABLE[i].as_string, cmd) == 0) {
            return i;
        }
    }

    return -1;
}