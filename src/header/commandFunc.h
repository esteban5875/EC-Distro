#pragma once

#include "shell.h"

namespace Shell {
    using CommandVoid = void(Command*);
    using CommandInt  = int(Command*);

    CommandVoid echo;

    CommandInt  builtInType;
    CommandInt  builtInChdir;
    CommandInt  builtInPwd;
}

#define DEFINE_COMMAND_VOID_RETURN(FuncName) void Shell::FuncName(Command* cmd)
#define DEFINE_COMMAND_INT_RETURN(FuncName)  int Shell::FuncName(Command* cmd)

#define EXPOSE_COMMAND_VOID(FuncName) inline Shell::CommandVoid* FuncName = &Shell::FuncName
#define EXPOSE_COMMAND_INT(FuncName)  inline Shell::CommandInt*  FuncName = &Shell::FuncName