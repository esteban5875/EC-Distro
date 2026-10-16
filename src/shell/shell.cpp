#include <sstream>
#include <unordered_map>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

#include "../header/shell.h"
#include "../header/typeData.h"

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

static std::vector<std::string> split(const std::string& s, char delim, bool keepEmpty = false) {
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

enum class FileCheck { Ok, Missing, NotRegular, NotExecutable };
static FileCheck checkFile(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return FileCheck::Missing;
    if (!S_ISREG(st.st_mode))         return FileCheck::NotRegular;
    if (access(path.c_str(), X_OK) != 0) return FileCheck::NotExecutable;
    return FileCheck::Ok;
}

LookupResult findInPath(const std::string& name) {
    if (name.find('/') != std::string::npos) {
        switch (checkFile(name)) {
            case FileCheck::Ok:            return {LookupStatus::Found, name};
            case FileCheck::NotRegular:    return {LookupStatus::NotRegular, name};
            case FileCheck::NotExecutable: return {LookupStatus::NotExecutable, name};
            case FileCheck::Missing:       return {LookupStatus::NotFound, ""};
        }
    }

    const char* env = getenv("PATH");
    if (!env) return {LookupStatus::PathUnset, ""};

    LookupResult firstProblem{LookupStatus::NotFound, ""};

    for (const std::string& dir : split(env, ':')) {
        if (dir.empty()) continue;
        std::string candidate = dir + "/" + name;

        switch (checkFile(candidate)) {
            case FileCheck::Ok:
                return {LookupStatus::Found, candidate}; 
            case FileCheck::NotExecutable:
                if (firstProblem.status == LookupStatus::NotFound)
                    firstProblem = {LookupStatus::NotExecutable, candidate};
                break;
            case FileCheck::NotRegular:
            case FileCheck::Missing:
                break;   
        }
    }
    return firstProblem;   
}

int parse(const std::string& line, Command* out) {
    std::vector<std::string> parsedCommand = splitBySpaces(line);
    if (parsedCommand.empty()) {
        return 1;   
    }

    auto it = CommandMap.find(parsedCommand[0]);
    CommandName commandName =
        (it != CommandMap.end()) ? it->second : CommandName::External;
    std::vector<std::string> args(parsedCommand.begin() + 1, parsedCommand.end());

    switch (commandName) {
    case CommandName::Echo:
        out->name = CommandName::Echo;
        break;
    case CommandName::Type:
        out->name = CommandName::Type;
        break;
    case CommandName::Pwd:
        out->name = CommandName::Pwd;
        break;
    case CommandName::Cd:
        out->name = CommandName::Cd;
        break;
    case CommandName::External:
        out->name = CommandName::External;
        out->enteredInput = parsedCommand[0];
        break;
    }

    out->args = std::move(args);
    return 0;
}