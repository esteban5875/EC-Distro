#pragma once

#include <string>
#include <vector>

enum class CommandName{
    Echo,
    Type,
    Unknown
};

struct Command
{
    const CommandName name;
    const std::vector<std::string> args;

    bool isBuiltin() const { return name != CommandName::Unknown; }
};

int parse(const std::string& line, Command* out);

