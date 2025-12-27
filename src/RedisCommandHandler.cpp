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

static std::string handleRenameCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 3) return "-Error: RENAME requires old_key and new_key\r\n";
    std::string old_key = tokens[1];
    std::string new_key = tokens[2];
    if (db.rename(old_key, new_key)) {
        return "+OK\r\n";
    } else {
        return "-Error: Failed to rename key\r\n";
    }
}

static std::string handleExpireCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 3) return "-Error: EXPIRE requires a key and seconds\r\n";
    std::string key = tokens[1];
    int seconds = std::stoi(tokens[2]);
    if (db.expire(key, seconds)) {
        return ":1\r\n";
    } else {
        return ":0\r\n";
    }
}

static std::string handleDelCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return "-Error: DEL requires a key\r\n";
    bool res = db.del(tokens[1]);
    if(res) return ":1\r\n";
    return ":0\r\n";
}

static std::string handleKeysCommand(RedisDatabase& db) {
    std::ostringstream response;
    std::vector<std::string> all_keys = db.keys();
    response << "*" << all_keys.size() << "\r\n";
    for (const auto& key : all_keys) {
        response << "$" << key.size() << "\r\n" << key << "\r\n";
    }
    return response.str();
}

static std::string handleTypeCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return "-Error: TYPE requires a key\r\n";
    std::string key = tokens[1];
    std::string value;
    if (db.get(key, value)) {
        return "+string\r\n";
    }
    auto list_value = db.lget(key);
    if (!list_value.empty()) {
        return "+list\r\n";
    }
    auto hash_value = db.hget(key);
    if (!hash_value.empty()) {
        return "+hash\r\n";
    }
    return "+none\r\n";
}

static std::string handleEchoCommand(const std::vector<std::string>& tokens) {
    if(tokens.size() < 2) return "-Error: ECHO requires a message\r\n";
    return "+" + tokens[1] + "\r\n";
}

static std::string handleGetCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return "-Error: GET requires a key\r\n";
    std::string value;
    if (db.get(tokens[1], value)) {
        return "+" + value + "\r\n";
    } else {
        return "+\r\n"; // Nil response
    }
}   

static std::string handleSetCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 3) return "-Error: SET requires a key and a value\r\n";
    db.set(tokens[1], tokens[2]);
    return "+OK\r\n";
}

static std::string handleLlenCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return "-Error: LLEN requires a key\r\n";
    std::string key = tokens[1];
    auto list_value = db.lget(key);
    std::ostringstream response;
    if (!list_value.empty()) {
        response << ":" << list_value.size() << "\r\n";
    } else {
        response << ":0\r\n";
    }
    return response.str();
}

static std::string handleFlushAllCommand(RedisDatabase& db) {
    if (db.flushAll()) {
        return "+OK\r\n";
    } else {
        return "-Error: Failed to flush database\r\n";
    }
}

static std::string handleDumpCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return "-Error: DUMP requires a filename\r\n";
    if (db.dump(tokens[1])) {
        return "+OK\r\n";
    } else {
        return "-Error: Failed to dump database\r\n";
    }
}

static std::string handleLoadCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return "-Error: LOAD requires a filename\r\n";
    if (db.load(tokens[1])) {
        return "+OK\r\n";
    } else {
        return "-Error: Failed to load database\r\n";
    }
}

static std::string handlePingCommand() {
    return "+PONG\r\n";
}

static std::string handleUnknownCommand() {
    return "-Error: Unknown command\r\n";
}

static std::string gandleRpushCommand(RedisDatabase& db, const std::vector<std::string>& tokens, bool left) {
    if (tokens.size() < 3) return left ? "-Error: LPUSH requires a key and a value\r\n" : "-Error: RPUSH requires a key and a value\r\n";
    std::string key = tokens[1];
    std::string value = tokens[2];
    if (left) {
        db.lpush(key, value);
    } else {
        db.rpush(key, value);
    }
    return "+OK\r\n";
}

static std::string handleLpopRpopCommand(RedisDatabase& db, const std::vector<std::string>& tokens, bool left) {
    if (tokens.size() < 2) return left ? "-Error: LPOP requires a key\r\n" : "-Error: RPOP requires a key\r\n";
    std::string key = tokens[1];
    std::string value;
    bool success = left ? db.lpop(key, value) : db.rpop(key, value);
    if (success) {
        return "+" + value + "\r\n";
    } else {
        return "+\r\n"; // Nil response
    }
}

static std::string handleLremCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 4) return "-Error: LREM requires a key, value, and count\r\n";
    std::string key = tokens[1];
    std::string value = tokens[2];
    int count = std::stoi(tokens[3]);
    int removed = db.lrem(key, value, count);
    return ":" + std::to_string(removed) + "\r\n";
}

static std::string handleLindexCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 3) return "-Error: LINDEX requires a key and index\r\n";
    std::string key = tokens[1];
    int index = std::stoi(tokens[2]);
    std::string value;
    if (db.lindex(key, index, value)) {
        return "+" + value + "\r\n";
    } else {
        return "+\r\n"; // Nil response
    }
}

static std::string handleLsetCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 4) return "-Error: LSET requires a key, index, and value\r\n";
    std::string key = tokens[1];
    int index = std::stoi(tokens[2]);
    std::string value = tokens[3];
    if (db.lset(key, index, value)) {
        return "+OK\r\n";
    } else {
        return "-Error: Failed to set list element\r\n";
    }
}

static std::string handleHgetCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return "-Error: HGET requires a key\r\n";
    std::string key = tokens[1];
    auto hash_value = db.hget(key);
    if (!hash_value.empty()) {
        std::ostringstream response;
        response << "*" << hash_value.size() << "\r\n";
        for (const auto& field_kv : hash_value) {
            response << "$" << field_kv.first.size() << "\r\n" << field_kv.first << "\r\n";
            response << "$" << field_kv.second.size() << "\r\n" << field_kv.second << "\r\n";
        }
        return response.str();
    } else {
        return "+\r\n"; // Nil response
    }
}

static std::string handleLpopRpopCommand(RedisDatabase& db, const std::vector<std::string>& tokens, bool left) {
    if (tokens.size() < 2) return left ? "-Error: LPOP requires a key\r\n" : "-Error: RPOP requires a key\r\n";
    std::string key = tokens[1];
    std::string value;
    bool success = left ? db.lpop(key, value) : db.rpop(key, value);
    if (success) {
        return "+" + value + "\r\n";
    } else {
        return "+\r\n"; // Nil response
    }
}

static std::string handleLremCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 4) return "-Error: LREM requires a key, value, and count\r\n";
    std::string key = tokens[1];
    std::string value = tokens[2];
    int count = std::stoi(tokens[3]);
    int removed = db.lrem(key, value, count);
    return ":" + std::to_string(removed) + "\r\n";
}

static std::string handleLsetCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 4) return "-Error: LSET requires a key, index, and value\r\n";
    std::string key = tokens[1];
    int index = std::stoi(tokens[2]);
    std::string value = tokens[3];
    if (db.lset(key, index, value)) {
        return "+OK\r\n";
    } else {
        return "-Error: Failed to set list element\r\n";
    }
}

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
    } else if (cmd == "FLUSHALL") {
        if (db.flushAll()) {
            return "+OK\r\n";
        } else {
            return "-Error: Failed to flush database\r\n";
        }
    } else if (cmd == "KEYS") {
        std::vector<std::string> all_keys = db.keys();
        response << "*" << all_keys.size() << "\r\n";
        for (const auto& key : all_keys) {
            response << "$" << key.size() << "\r\n" << key << "\r\n";
        }
    } else if (cmd == "TYPE") {
        if (tokens.size() < 2) return "-Error: TYPE requires a key\r\n";
        std::string key = tokens[1];
        std::string value;
        if (db.get(key, value)) {
            return "+string\r\n";
        }
        auto list_value = db.lget(key);
        if (!list_value.empty()) {
            return "+list\r\n";
        }
        auto hash_value = db.hget(key);
        if (!hash_value.empty()) {
            return "+hash\r\n";
        }
        return "+none\r\n";
    } else if (cmd == "DEL") {
        if (tokens.size() < 2) return "-Error: DEL requires a key\r\n";
        std::string key = tokens[1];
        std::string value;
        if (db.get(key, value)) {
            db.set(key, ""); // Simula a deleção
            return ":1\r\n";
        }
        auto list_value = db.lget(key);
        if (!list_value.empty()) {
            db.lset(key, {}); // Simula a deleção
            return ":1\r\n";
        }
        auto hash_value = db.hget(key);
        if (!hash_value.empty()) {
            db.hset(key, {}); // Simula a deleção
            return ":1\r\n";
        }
        else {
            bool res = db.del(tokens[1]);
            if(res) return ":1\r\n";
        }
        return ":0\r\n";
    } else if(cmd == "ECHO") {
        if(tokens.size() < 2) return "-Error: ECHO requires a message\r\n";
        return "+" + tokens[1] + "\r\n";
    } else if (cmd == "SET") {
        if (tokens.size() < 3) return "-Error: SET requires a key and a value\r\n";
        // Implementar lógica de SET aqui
        return "+OK\r\n";
      } else if (cmd == "GET") {
        if (tokens.size() < 2) return "-Error: GET requires a key\r\n";
        // Implementar lógica de GET aqui
        std::string value;
        if (db.get(tokens[1], value)) {
            response << "+" << value << "\r\n";
        } else {
            std::string value;
            if(db.get(tokens[1], value))
            response << "+"+tokens[2]<<"\r\n";
        }
        return "+value\r\n"; // Substituir "value" pelo valor real
    }else if (cmd == "KEYS") {
        auto all_keys = db.keys();
        response << "*" << all_keys.size() << "\r\n";
} else if (cmd == "EXPIRE") {
        if (tokens.size() < 3) return "-Error: EXPIRE requires a key and seconds\r\n";
        std::string key = tokens[1];
        int seconds = std::stoi(tokens[2]);
        if (db.expire(key, seconds)) { // Fixed typo: expire
            return ":1\r\n";
        } else {
            return ":0\r\n";
        }
    } else if (cmd == "RENAME") { // Moved this above the final 'else'
        if (tokens.size() < 3) return "-Error: RENAME requires old_key and new_key\r\n";
        std::string old_key = tokens[1];
        std::string new_key = tokens[2];
        if (db.rename(old_key, new_key)) {
            return "+OK\r\n";
        } else {
            return "-Error: Failed to rename key\r\n";
        }
    } else if (cmd == "LLen") {
        if (tokens.size() < 2) return "-Error: LLEN requires a key\r\n";
        std::string key = tokens[1];
        auto list_value = db.lget(key);
        if (!list_value.empty()) {
            response << ":" << list_value.size() << "\r\n";
        } else {
            response << ":0\r\n";
        }

    } else {
        return "-Error: Unknown command\r\n";
    }

    return response.str();
}
  