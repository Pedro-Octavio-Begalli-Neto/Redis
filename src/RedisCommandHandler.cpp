#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"
#include <vector>
#include <sstream>
#include <algorithm>
#include <iostream>

std::vector<std::string> parseRespCommand(const std::string& input) {
    std::vector<std::string> tokens;
    if(input.empty()) return tokens;

    if (input[0] != '*') {
        std::istringstream iss(input);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }

    size_t pos = 0;
    pos++; // Skip '*'

    size_t crlf = input.find("\r\n", pos);
    if (crlf == std::string::npos) return tokens;
    int numElements = std::stoi(input.substr(pos, crlf - pos));
    pos = crlf + 2;

    for (int i = 0; i < numElements; ++i) {
        if (pos >= input.size() || input[pos] != '$') break;
        pos++; // Skip '$'

        crlf = input.find("\r\n", pos);
        if (crlf == std::string::npos) break;
        int Len = std::stoi(input.substr(pos, crlf - pos));
        pos = crlf + 2;

        if (pos + Len > input.size()) break;
        tokens.push_back(input.substr(pos, Len));
        pos += Len + 2;
    }
    return tokens;
} // Removido o ';' que estava aqui

RedisCommandHandler::RedisCommandHandler() {}

std::string RedisCommandHandler::handleCommand(const std::string& commandLine) {
    auto tokens = parseRespCommand(commandLine);
    if (tokens.empty()) {
        return "-Error: Empty command\r\n";
    }

    for(auto& t : tokens) {
        std::cout << "Token: " << t << "\n";
    }

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    std::ostringstream response;
    RedisDatabase& db = RedisDatabase::getInstance();
    
    // Resposta padrão para teste
    if (cmd == "PING") return "+PONG\r\n";
    else if (cmd == "DUMP") {
        if (tokens.size() < 2) return "-Error: DUMP requires a filename\r\n";
        if (db.dump(tokens[1])) {
            return "+OK\r\n";
        } else {
            return "-Error: Failed to dump database\r\n";
        }
    } else if (cmd == "LOAD") {
        if (tokens.size() < 2) return "-Error: LOAD requires a filename\r\n";
        if (db.load(tokens[1])) {
            return "+OK\r\n";
        } else {
            return "-Error: Failed to load database\r\n";
        }
    }
    
    return "+OK\r\n";
}
    