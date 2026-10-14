#pragma once

#include "shell.h"

#define DECLARE_COMMAND(FuncName, ReturnType) ReturnType FuncName(Command* cmd)

DECLARE_COMMAND(echo, void);
DECLARE_COMMAND(builtIntype, int);