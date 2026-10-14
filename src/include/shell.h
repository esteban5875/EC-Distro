#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <type_traits>

enum class CommandName{Echo, Type, Unknown};

static constexpr std::string_view commandToString(CommandName c){
    switch (c) {
        case CommandName::Echo: return "Echo";
        case CommandName::Type: return "Type";
        default:                return "Unknown";        
    }
}

struct Command
{
    CommandName name;
    std::vector<std::string> args;
    std::string enteredInput; //Only if command unknown for debugging

    bool isBuiltin() const { return name != CommandName::Unknown; }

    std::string toString() const {
        return std::string(commandToString(name)); 
    }
};


template <typename CommandFunc>
using EnableIfCommandFunc = std::enable_if_t<std::is_invocable_r_v<bool, CommandFunc, Command*>>;


int parse(const std::string& line, Command* out);
void parsePath(const std::string& line, std::vector<std::string>* out);
std::string findInPath(const std::string& name);
