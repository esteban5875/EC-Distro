#pragma once

#include <stddef.h>

typedef int (*cmd_method) (const char** args, const size_t argnum);

typedef enum {
    ECHO,
    NONE
} BUILT_IN_COMMAND;

typedef struct {
    const char*  command;
    const char** args;
    const size_t argsNum;
} COMMAND_INPUT;

typedef struct {
    BUILT_IN_COMMAND name;
    cmd_method       method;
    const char*      as_string;
    const size_t     max_args_num;
} LOOKUP_COMMAND;

COMMAND_INPUT    Lexer(const char* input);
BUILT_IN_COMMAND Parse(COMMAND_INPUT* cmd_in);
int              Exec (BUILT_IN_COMMAND* cmd);