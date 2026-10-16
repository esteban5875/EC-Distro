#include <iostream>

#include "../header/commandFunc.h"

static std::string unescape(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            switch (s[++i]) {
                case 'n':  out += '\n'; break;
                case 't':  out += '\t'; break;
                case 'r':  out += '\r'; break;
                case '\\': out += '\\'; break;
                case '"':  out += '"';  break;
                case '0':  out += '\0'; break;
                default:   out += '\\'; out += s[i]; break;  // unknown: keep as-is
            }
        } else {
            out += s[i];
        }
    }
    return out;
}

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

DEFINE_COMMAND_VOID_RETURN(echo){
    std::cout << unescape(joinBySpaces(cmd->args));
}