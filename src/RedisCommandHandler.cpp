#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"
#include <vector>
#include <sstream>
#include <algorithm>
#include <iostream>

// --- Helper Parser ---
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
}

// --- Static Command Handlers ---

static std::string handlePingCommand() {
    return "+PONG\r\n";
}

static std::string handleEchoCommand(const std::vector<std::string>& tokens) {
    if(tokens.size() < 2) return "-Error: ECHO requires a message\r\n";
    return "+" + tokens[1] + "\r\n";
}

static std::string handleSetCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 3) return "-Error: SET requires a key and a value\r\n";
    db.set(tokens[1], tokens[2]);
    return "+OK\r\n";
}

static std::string handleGetCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return "-Error: GET requires a key\r\n";
    std::string value;
    if (db.get(tokens[1], value)) {
        return "+" + value + "\r\n";
    }
    return "+\r\n"; // Nil
}

static std::string handleDelCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return "-Error: DEL requires a key\r\n";
    return db.del(tokens[1]) ? ":1\r\n" : ":0\r\n";
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
    if (db.get(key, value)) return "+string\r\n";
    if (!db.lget(key).empty()) return "+list\r\n";
    if (!db.hget(key).empty()) return "+hash\r\n";
    return "+none\r\n";
}

static std::string handleRenameCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 3) return "-Error: RENAME requires old_key and new_key\r\n";
    return db.rename(tokens[1], tokens[2]) ? "+OK\r\n" : "-Error: Failed to rename key\r\n";
}

static std::string handleExpireCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 3) return "-Error: EXPIRE requires a key and seconds\r\n";
    try {
        int seconds = std::stoi(tokens[2]);
        return db.expire(tokens[1], seconds) ? ":1\r\n" : ":0\r\n";
    } catch (...) { return "-Error: Invalid seconds\r\n"; }
}

// --- List Handlers ---

static std::string handleRpushLpushCommand(RedisDatabase& db, const std::vector<std::string>& tokens, bool left) {
    if (tokens.size() < 3) return "-Error: Key and value required\r\n";
    left ? db.lpush(tokens[1], tokens[2]) : db.rpush(tokens[1], tokens[2]);
    return "+OK\r\n";
}

static std::string handleLpopRpopCommand(RedisDatabase& db, const std::vector<std::string>& tokens, bool left) {
    if (tokens.size() < 2) return "-Error: Key required\r\n";
    std::string value;
    bool success = left ? db.lpop(tokens[1], value) : db.rpop(tokens[1], value);
    return success ? "+" + value + "\r\n" : "+\r\n";
}

static std::string handleLlenCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return "-Error: LLEN requires a key\r\n";
    auto list_value = db.lget(tokens[1]);
    return ":" + std::to_string(list_value.size()) + "\r\n";
}

static std::string handleLindexCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 3) return "-Error: LINDEX requires a key and index\r\n";
    std::string value;
    if (db.lindex(tokens[1], std::stoi(tokens[2]), value)) return "+" + value + "\r\n";
    return "+\r\n";
}

// --- Hash Handlers ---

static std::string handleHsetCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 4) return "-Error: HSET requires a key, field, and value\r\n";
    
    std::string key = tokens[1];
    // Create the map the database is looking for
    std::unordered_map<std::string, std::string> data;
    data[tokens[2]] = tokens[3]; 

    // Now the types match!
    db.hset(key, data); 
    
    return ":1\r\n"; 
}

static std::string handleHkeyCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 3) return "-Error: HKEY requires a key and field\r\n";
    std::string key = tokens[1];
};

static std::string handleHgetCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return "-Error: HGET requires a key\r\n";
    auto hash_value = db.hget(tokens[1]);
    if (hash_value.empty()) return "+\r\n";
    
    std::ostringstream response;
    response << "*" << hash_value.size() * 2 << "\r\n";
    for (const auto& kv : hash_value) {
        response << "$" << kv.first.size() << "\r\n" << kv.first << "\r\n";
        response << "$" << kv.second.size() << "\r\n" << kv.second << "\r\n";
    }
    return response.str();
}

static std::string HvalsCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return "-Error: HVALS requires a key\r\n";
    auto hash_value = db.hvals(tokens[1]);
    if (hash_value.empty()) return "+\r\n";
    
    std::ostringstream response;
    response << "*" << hash_value.size() << "\r\n";
    for (const auto& val : hash_value) {
        response << "$" << val.size() << "\r\n" << val << "\r\n";
    }
    return response.str();
}

// --- System Handlers ---

static std::string handleFlushAllCommand(RedisDatabase& db) {
    return db.flushAll() ? "+OK\r\n" : "-Error: Failed to flush\r\n";
}

static std::string handleDumpCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return "-Error: DUMP requires a filename\r\n";
    return db.dump(tokens[1]) ? "+OK\r\n" : "-Error: Failed to dump\r\n";
}

static std::string handleLoadCommand(RedisDatabase& db, const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return "-Error: LOAD requires a filename\r\n";
    return db.load(tokens[1]) ? "+OK\r\n" : "-Error: Failed to load\r\n";
}

// --- Main Handler Class Implementation ---

RedisCommandHandler::RedisCommandHandler() {}

std::string RedisCommandHandler::handleCommand(const std::string& commandLine) {
    auto tokens = parseRespCommand(commandLine);
    if (tokens.empty()) return "-Error: Empty command\r\n";

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    
    RedisDatabase& db = RedisDatabase::getInstance();

    if (cmd == "PING") return handlePingCommand();
    if (cmd == "ECHO") return handleEchoCommand(tokens);
    if (cmd == "SET")  return handleSetCommand(db, tokens);
    if (cmd == "GET")  return handleGetCommand(db, tokens);
    if (cmd == "DEL")  return handleDelCommand(db, tokens);
    if (cmd == "KEYS") return handleKeysCommand(db);
    if (cmd == "TYPE") return handleTypeCommand(db, tokens);
    if (cmd == "RENAME") return handleRenameCommand(db, tokens);
    if (cmd == "EXPIRE") return handleExpireCommand(db, tokens);
    
    // Lists
    if (cmd == "LPUSH") return handleRpushLpushCommand(db, tokens, true);
    if (cmd == "RPUSH") return handleRpushLpushCommand(db, tokens, false);
    if (cmd == "LPOP")  return handleLpopRpopCommand(db, tokens, true);
    if (cmd == "RPOP")  return handleLpopRpopCommand(db, tokens, false);
    if (cmd == "LLEN")  return handleLlenCommand(db, tokens);
    if (cmd == "LINDEX") return handleLindexCommand(db, tokens);
    
    // Hashes
    if (cmd == "HSET") return handleHsetCommand(db, tokens);
    if (cmd == "HGET") return handleHgetCommand(db, tokens);

    // System
    if (cmd == "FLUSHALL") return handleFlushAllCommand(db);
    if (cmd == "DUMP")     return handleDumpCommand(db, tokens);
    if (cmd == "LOAD")     return handleLoadCommand(db, tokens);

    return "-Error: Unknown command\r\n";
}