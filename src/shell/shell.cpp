#include "../include/shell.h"
#include "../include/echo.h"

#include <sstream>
#include <unordered_map>
#include <iostream>

static std::unordered_map<std::string, CommandName> CommandMap = {
    {"Echo", CommandName::Echo},
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

int parse(const std::string& line, Command* out){

    if (line == ""){
        return 0;
    }

    std::vector<std::string> parsedCommand = splitBySpaces(line);

    auto it = CommandMap.find(parsedCommand[0]);
    CommandName commandName = (it != CommandMap.end()) ? it->second : CommandName::Unknown;
    std::vector<std::string> args(parsedCommand.begin() + 1, parsedCommand.end());

    switch (commandName)
    {
    case CommandName::Echo:{
        echo(args);
        break;
    }

    case CommandName::Unknown:
        return 1;
    }
}