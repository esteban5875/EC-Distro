#pragma once

#include "db.h"

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
bool cmdList(const std::vector<dbEntry> &db);
bool cmdInfo(const std::vector<dbEntry> &db, const std::string &name);
bool cmdInstall(std::vector<dbEntry> &db, const std::string &name);
bool cmdRemove(std::vector<dbEntry> &db, const std::string &name);

// Loads the db and dispatches to the command in `args`.
bool run(const Args &args);
