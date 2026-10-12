#include "../include/echo.h"

#include <iostream>

static std::string joinBySpaces(const std::vector<std::string>& words) {
    if (words.empty()) {
        return "";
    }

    std::string result;
    
    size_t total_length = 0;
    for (const auto& word : words) {
        total_length += word.length() + 1;
    }
    result.reserve(total_length);

    for (size_t i = 0; i < words.size(); ++i) {
        result += words[i];
        if (i < words.size() - 1) {
            result += ' ';
        }
    }

    return result;
}

void echo(const std::vector<std::string>& args){
    std::cout << joinBySpaces(args);
};
