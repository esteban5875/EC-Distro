#pragma once

#include "shell.h"
#include <functional>

namespace Shell {
    using CommandVoid = void(Command*);
    using CommandInt  = int(Command*);

    CommandVoid echo;
    CommandInt  builtInType;
}

#define DEFINE_COMMAND_VOID_RETURN(FuncName) void Shell::FuncName(Command* cmd)
#define DEFINE_COMMAND_INT_RETURN(FuncName)  int Shell::FuncName(Command* cmd)

#define DECLARE_COMMAND_LINK_VOID(FuncName) inline Shell::CommandVoid* FuncName = &Shell::FuncName
#define DECLARE_COMMAND_LINK_INT(FuncName)  inline Shell::CommandInt*  FuncName = &Shell::FuncName