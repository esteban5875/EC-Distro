#include "../include/shell.h"
#include "../include/echo.h"

#include <sstream>
#include <unordered_map>
#include <iostream>

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
        break;
    }
    return 0;
}