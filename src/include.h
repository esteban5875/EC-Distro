#pragma once

#include "./header/shell.h"
#include "./header/commandFunc.h"

//Void Return Commands (Direct Action)

EXPOSE_COMMAND_VOID(echo);

//Int Return Commands (Error Handling)

EXPOSE_COMMAND_INT(builtInType);