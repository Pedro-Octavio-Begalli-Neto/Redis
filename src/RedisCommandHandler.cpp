#include <include/RedisCommandHandler.h>
#include <vector>
#include <sstream>
#include <algorithm>

std::vector<std::string> parseRespCommand(const std::string& input) {
    std::vector<std::string> tokens;
    if(input.empty()) return tokens;

    // If the input starts with '*', it's an array in RESP
    if (input[0] != '*') {
        std::istringstream iss(input);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens; // Invalid RESP array
    }

    size_t pos = 0;
    if (input[pos] != '*') return tokens;
    pos++;// Skip '*'

    size_t crlf = input.find("\r\n", pos);
    if (crlf == std::string::npos) return tokens;
    int numElements = std::stoi(input.substr(pos, crlf - pos));
    pos = crlf + 2;

    for (int i = 0; i < numElements; ++i) {
        if (pos >= input.size() || input[pos] != '$') break;;// Invalid format
        pos++; // Skip '$'

        crlf = input.find("\r\n", pos);
        if (crlf == std::string::npos) break;;
        int Len = std::stoi(input.substr(pos, crlf - pos));
        pos = crlf + 2;

        if (pos + Len > input.size()) break;;// Invalid length
        tokens.push_back(input.substr(pos, Len));
        pos += Len + 2; // Skip string and CRLF
    }
    return tokens;
};

RedisCommandHandler::RedisCommandHandler() {}
std::string RedisCommandHandler::handleCommand(const std::string& commandLine) {
    auto tokens = parseRespCommand(commandLine);
    if (tokens.empty()) {
        return "-Error: Empty command\r\n";
    }

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    std::ostringstream response;



    return response.str();
}
    