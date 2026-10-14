#pragma once

#include "./header/shell.h"
#include "./header/commandFunc.h"

//Void Return Commands (Direct Action)

DECLARE_COMMAND_LINK_VOID(echo);

//Int Return Commands (Error Handling)

DECLARE_COMMAND_LINK_INT(builtInType);