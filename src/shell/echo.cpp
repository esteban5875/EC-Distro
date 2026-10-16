#include <iostream>
#include <cstring>

#include "../header/builtInCommands.h"
#include "../header/constants.h"

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

int builtInEcho(Command* cmd, char* buf){
    size_t len = 0;                                   

    auto flush = [&]() {
        std::cout.write(buf, len);  
        len = 0;
    };

    auto append = [&](const char* data, size_t n) {
        while (n > 0) {
            if (len == sizeof(buf)) flush();       
            size_t space = sizeof(buf) - len;
            size_t chunk = n < space ? n : space;
            std::memcpy(buf + len, data, chunk);
            len += chunk;
            data += chunk;
            n -= chunk;
        }
    };

    for (size_t i = 0; i < cmd->args.size(); ++i) {
        if (i > 0) append(" ", 1);
        append(cmd->args[i].data(), cmd->args[i].size());
    }

    append("\n", 1);
    flush();
    std::cout.flush();
    return 0;
}