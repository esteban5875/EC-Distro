#pragma once

#include "./shell.h"

int builtInChdir(Command* cmd);
int builtInEcho(Command* cmd, char* buf);
int builtInPwd();
int builtInType(Command* cmd);
