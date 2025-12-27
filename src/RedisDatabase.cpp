#include "../include/RedisDatabase.h"
#include <iostream>
#include <mutex>
std::mutex db_mutex;
#include <fstream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <string>

RedisDatabase& RedisDatabase::getInstance() {
    static RedisDatabase instance;
    return instance;
}

bool RedisDatabase::flushAll() {
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store.clear();
    list_store.clear();
    hash_store.clear();
    return true;
}

void RedisDatabase::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store[key] = value;
}

bool RedisDatabase::get(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = kv_store.find(key);
    if (it != kv_store.end()) {
        value = it->second;
        return true;
    }
    return false;
}

std::vector<std::string> RedisDatabase::lget(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it != list_store.end()) {
        return it->second;
    }
    return {};
}
std::vector<std::string> RedisDatabase::keys() {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::vector<std::string> all_keys;
    for (const auto& kv : kv_store) {
        all_keys.push_back(kv.first);
    }
    for (const auto& kv : list_store) {
        all_keys.push_back(kv.first);
    }
    for (const auto& kv : hash_store) {
        all_keys.push_back(kv.first);
    }
    return all_keys;
}

std::string RedisDatabase::type(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (kv_store.find(key) != kv_store.end()) {
        return "string";
    }
    if (list_store.find(key) != list_store.end()) {
        return "list";
    }
    if (hash_store.find(key) != hash_store.end()) {
        return "hash";
    }
    return "none";
}   

std::string hget(const std::string& key);

bool RedisDatabase::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    size_t erased = 0;
    erased += kv_store.erase(key);
    erased += list_store.erase(key);
    erased += hash_store.erase(key);
    return erased > 0;
} 

bool RedisDatabase::expire(const std::string& key, int seconds) {
    std::lock_guard<std::mutex> lock(db_mutex);
    time_t expire_time = std::time(nullptr) + seconds;
    expire_times[key] = expire_time;
    expire_clocks[key] = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    return true;
}

bool RedisDatabase::rename(const std::string& old_key, const std::string& new_key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto itKV = kv_store.find(old_key);
    if (itKV != kv_store.end()) {
        kv_store[new_key] = itKV->second;
        kv_store.erase(itKV);
        return true;
    }

    auto itList = list_store.find(old_key);
    if (itList != list_store.end()) {
        list_store[new_key] = itList->second;
        list_store.erase(itList);
        return true;
    }

    auto itHash = hash_store.find(old_key);
    if (itHash != hash_store.end()) {   
        hash_store[new_key] = itHash->second;
        hash_store.erase(itHash);
        return true;
    }

    auto itExpire = expire_times.find(old_key);
    if (itExpire != expire_times.end()) {
        expire_times[new_key] = itExpire->second;
        expire_times.erase(itExpire);
    }
    // Verifica se a chave antiga existe em qualquer uma das lojas
    if (kv_store.find(old_key) != kv_store.end()) {
        kv_store[new_key] = kv_store[old_key];
        kv_store.erase(old_key);
        return true;
    }
    if (list_store.find(old_key) != list_store.end()) {
        list_store[new_key] = list_store[old_key];
        list_store.erase(old_key);
        return true;
    }
    if (hash_store.find(old_key) != hash_store.end()) {
        hash_store[new_key] = hash_store[old_key];
        hash_store.erase(old_key);
        return true;
    }
    return false; // Chave antiga não encontrada
}   

bool RedisDatabase::dump(const std::string& filename) {
    // Implementação fictícia de dump
    // Aqui você adicionaria o código real para salvar o estado do banco de dados em um arquivo
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) {
        std::cerr << "Erro ao abrir o arquivo para dump: " << filename << std::endl;
        return false;
    }
    for (const auto& kv : kv_store) {
        ofs << kv.first << "\n" << kv.second << "\n";
    }
    for (const auto& list_kv : list_store) {
        ofs << list_kv.first << "\n";
        for (const auto& item : list_kv.second) {
            ofs << item << "\n";
        }
        ofs << "END_OF_LIST\n";
    }
    for (const auto& hash_kv : hash_store) {
        ofs << hash_kv.first << "\n";
        for (const auto& field_kv : hash_kv.second) {
            ofs << field_kv.first << "\n" << field_kv.second << "\n";
        }
        ofs << "END_OF_HASH\n";
    }
    for (const auto& kv : kv_store) {
        ofs << kv.first << "\n" << kv.second << "\n";
    }
    return true; // Retorna true se o dump for bem-sucedido
}

bool RedisDatabase::load(const std::string& filename) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) {
        std::cerr << "Erro ao abrir o arquivo para load: " << filename << std::endl;
        return false;
    }
    kv_store.clear();
    list_store.clear();
    hash_store.clear();

    std::string line;
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        char type;
        iss >> type;
        if (type == 'K') {
            std::string key, value;
            iss >> key;
            std::getline(ifs, value);
            kv_store[key] = value;
        } else if (type == 'L') {
            std::string key;
            iss >> key;
            std::vector<std::string> list;
            while (std::getline(ifs, line) && line != "END_OF_LIST") {
                list.push_back(line);
            }
            list_store[key] = list;
        } else if (type == 'H') {
            std::string key;
            iss >> key;
            std::unordered_map<std::string, std::string> hash;
            while (std::getline(ifs, line) && line != "END_OF_HASH") {
                std::string field = line;
                std::string value;
                std::getline(ifs, value);
                hash[field] = value;
            }
            hash_store[key] = hash;
        }
    }
    // Implementação fictícia de load
    // Aqui você adicionaria o código real para carregar o estado do banco de dados de um arquivo
    std::cout << "Loading database from " << filename << std::endl;
    return true; // Retorna true se o load for bem-sucedido
}