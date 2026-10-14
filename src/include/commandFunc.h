#pragma once

#include "shell.h"

#define DECLARE_COMMAND(FuncName) int FuncName(Command* cmd)

DECLARE_COMMAND(echo);
DECLARE_COMMAND(builtIntype);