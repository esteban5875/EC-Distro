#include "../header/shell.h"

#include <sstream>
#include <unordered_map>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

static std::unordered_map<std::string, CommandName> CommandMap = {
    {"echo", CommandName::Echo},
};

static std::vector<std::string> splitBySpaces(const std::string& str) {
    std::istringstream iss(str);
    std::vector<std::string> tokens;
    std::string token;

    while (iss >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

static bool isExecutableFile(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode)
           && access(path.c_str(), X_OK) == 0;
}

std::vector<std::string> split(const std::string& s, char delim, bool keepEmpty = false) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : s) {
        if (c == delim) {
            if (keepEmpty || !current.empty()) parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    if (keepEmpty || !current.empty()) parts.push_back(current);
    return parts;
}

std::string findInPath(const std::string& name) {
    if (name.find('/') != std::string::npos)          // has a slash: skip the search
        return isExecutableFile(name) ? name : "";

    const char* env = getenv("PATH");
    if (!env) return "";

    for (const std::string& dir : split(env, ':')) {
        if (dir.empty()) continue;
        std::string candidate = dir + "/" + name;
        if (isExecutableFile(candidate)) return candidate;   // first match wins
    }
    return "";
}

int parse(const std::string& line, Command* out) {
    std::vector<std::string> parsedCommand = splitBySpaces(line);
    if (parsedCommand.empty()) {
        return 1;   
    }

    auto it = CommandMap.find(parsedCommand[0]);
    CommandName commandName =
        (it != CommandMap.end()) ? it->second : CommandName::Unknown;
    std::vector<std::string> args(parsedCommand.begin() + 1, parsedCommand.end());

    switch (commandName) {
    case CommandName::Echo:
        out->name = CommandName::Echo;
        out->args = std::move(args);
        break;
    case CommandName::Type:
        out->name = CommandName::Type;
        out->args = std::move(args);
        break;
    case CommandName::Unknown:
        out->name = CommandName::Unknown;
        out->enteredInput = parsedCommand[0];
        out->args = std::move(args);
        break;
    }

    return 0;
}