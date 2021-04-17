#pragma once

#include <string>
#include <vector>

namespace nwtrees
{
    struct Error
    {
        enum Code
        {
            Unknown,
        } code;

        std::vector<std::string> messages;

        Error(const Code code_) : code(code_) { }
        Error(const Code code_, const std::string& message_) : code(code_), messages({message_}) { }
        Error(const Code code_, const std::vector<std::string>& messages_) : code(code_), messages(messages_) { }
    };
}
