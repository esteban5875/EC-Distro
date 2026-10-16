#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

enum class CommandName { Echo, Type, Clear, Exit, Pwd, External };

enum class LookupStatus { Found, NotFound, NotExecutable, NotRegular, PathUnset };

struct LookupResult {
    LookupStatus status;
    std::string  path;
};

inline const std::unordered_map<std::string, CommandName> CommandMap = {
    {"echo", CommandName::Echo},
    {"type", CommandName::Type},
    {"clear", CommandName::Clear},
    {"exit", CommandName::Exit},
    {"pwd",  CommandName::Pwd}
};

struct Command {
    CommandName name = CommandName::External;
    std::string enteredInput;            // what the user typed as the command name
    std::vector<std::string> args;

    bool isBuiltin() const { return name != CommandName::External; }
};

int getWorkingDir();
int parse(const std::string& line, Command* out);
void parsePath(const std::string& line, std::vector<std::string>* out);
LookupResult findInPath(const std::string& name);
