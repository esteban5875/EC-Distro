#pragma once

#include <string>
#include <vector>

enum class Command {
    List,
    Info,
    Install,
    Remove,
};

struct Args {
    Command     command;
    std::string name;      // empty for `list`
};


bool parseArgs(int argc, char **argv, Args &out);

bool usage();

// Each command returns the process exit code: 0 on success, 1 on failure.

// Loads the db and dispatches to the command in `args`.
bool run(const Args &args);
