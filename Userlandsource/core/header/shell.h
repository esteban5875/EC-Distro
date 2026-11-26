#pragma once

#include <stddef.h>

#define SHELL_OUT_BUFFER char[4096]
#define SHELL_IN_BUFFER char[4096]

typedef int (*cmd_method) (const COMMAND_INPUT);

typedef enum {
    ALIAS,
    NONE
} BUILT_IN_COMMAND;

typedef struct {
    const size_t size;
    const char*  data;
} STRING;

typedef struct {
    STRING  command;
    STRING* args;
    size_t  args_num;
} COMMAND_INPUT;

typedef struct {
    BUILT_IN_COMMAND name;
    int             args_max_num;
    const char*     usage;
    const cmd_method      method;
} COMMAND;

BUILT_IN_COMMAND Parse(const COMMAND_INPUT cmd_in);
int              Exec (const  BUILT_IN_COMMAND cmd);

