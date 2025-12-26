#include "../include/RedisDatabase.h"
#include <iostream>
#include <mutex>
std::mutex db_mutex;
#include <fstream>
#include <sstream>



RedisDatabase& RedisDatabase::getInstance() {
    static RedisDatabase instance;
    return instance;
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