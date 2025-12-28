#include "../include/RedisDatabase.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <iterator>

// Note: Ensure std::mutex db_mutex is declared in your .h file as a private member
// and not as a global variable to ensure proper encapsulation.

RedisDatabase& RedisDatabase::getInstance() {
    static RedisDatabase instance;
    return instance;
}

bool RedisDatabase::flushAll() {
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store.clear();
    list_store.clear();
    hash_store.clear();
    expire_times.clear();
    expire_clocks.clear();
    return true;
}

// --- String Operations ---

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

// --- Key Management ---

std::vector<std::string> RedisDatabase::keys() {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::vector<std::string> all_keys;
    for (const auto& kv : kv_store) all_keys.push_back(kv.first);
    for (const auto& kv : list_store) all_keys.push_back(kv.first);
    for (const auto& kv : hash_store) all_keys.push_back(kv.first);
    return all_keys;
}

std::string RedisDatabase::type(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (kv_store.count(key)) return "string";
    if (list_store.count(key)) return "list";
    if (hash_store.count(key)) return "hash";
    return "none";
}

bool RedisDatabase::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    bool erased = false;
    
    if (kv_store.erase(key)) erased = true;
    if (list_store.erase(key)) erased = true;
    if (hash_store.erase(key)) erased = true;
    
    expire_times.erase(key);
    expire_clocks.erase(key);
    
    return erased;
}

bool RedisDatabase::rename(const std::string& old_key, const std::string& new_key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    bool found = false;

    // Move Data
    if (auto it = kv_store.find(old_key); it != kv_store.end()) {
        kv_store[new_key] = std::move(it->second);
        kv_store.erase(it);
        found = true;
    } else if (auto it = list_store.find(old_key); it != list_store.end()) {
        list_store[new_key] = std::move(it->second);
        list_store.erase(it);
        found = true;
    } else if (auto it = hash_store.find(old_key); it != hash_store.end()) {
        hash_store[new_key] = std::move(it->second);
        hash_store.erase(it);
        found = true;
    }

    if (!found) return false;

    // Move Expiration if it exists
    if (auto it = expire_times.find(old_key); it != expire_times.end()) {
        expire_times[new_key] = it->second;
        expire_times.erase(it);
    }
    if (auto it = expire_clocks.find(old_key); it != expire_clocks.end()) {
        expire_clocks[new_key] = it->second;
        expire_clocks.erase(it);
    }

    return true;
}

// --- Expiration ---

bool RedisDatabase::expire(const std::string& key, int seconds) {
    std::lock_guard<std::mutex> lock(db_mutex);
    // Check if key exists in any store
    if (kv_store.count(key) || list_store.count(key) || hash_store.count(key)) {
        expire_times[key] = std::time(nullptr) + seconds;
        expire_clocks[key] = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
        return true;
    }
    return false;
}

bool RedisDatabase::expiretime(const std::string& key, time_t& expire_time) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = expire_times.find(key);
    if (it != expire_times.end()) {
        expire_time = it->second;
        return true;
    }
    return false;
}

// --- List Operations ---

void RedisDatabase::lpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    list_store[key].insert(list_store[key].begin(), value);
}

void RedisDatabase::rpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    list_store[key].push_back(value);
}

bool RedisDatabase::lpop(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it != list_store.end() && !it->second.empty()) {
        value = it->second.front();
        it->second.erase(it->second.begin());
        if (it->second.empty()) list_store.erase(it);
        return true;
    }
    return false;
}

bool RedisDatabase::rpop(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it != list_store.end() && !it->second.empty()) {
        value = it->second.back();
        it->second.pop_back();
        if (it->second.empty()) list_store.erase(it);
        return true;
    }
    return false;
}

ssize_t RedisDatabase::llen(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    return (it != list_store.end()) ? static_cast<ssize_t>(it->second.size()) : -1;
}

int RedisDatabase::lrem(const std::string& key, const std::string& value, int count) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it == list_store.end()) return 0;

    auto& lst = it->second;
    int removed = 0;

    if (count == 0) {
        auto new_end = std::remove(lst.begin(), lst.end(), value);
        removed = std::distance(new_end, lst.end());
        lst.erase(new_end, lst.end());
    } else if (count > 0) {
        for (auto iter = lst.begin(); iter != lst.end() && removed < count; ) {
            if (*iter == value) {
                iter = lst.erase(iter);
                ++removed;
            } else ++iter;
        }
    } else {
        int target = -count;
        for (auto iter = lst.rbegin(); iter != lst.rend() && removed < target; ) {
            if (*iter == value) {
                iter = std::reverse_iterator(lst.erase(std::next(iter).base()));
                ++removed;
            } else ++iter;
        }
    }

    if (lst.empty()) list_store.erase(it);
    return removed;
}

bool RedisDatabase::lindex(const std::string& key, int index, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it == list_store.end()) return false;
    
    const auto& lst = it->second;
    int size = static_cast<int>(lst.size());
    if (index < 0) index += size;
    
    if (index >= 0 && index < size) {
        value = lst[index];
        return true;
    }
    return false;
}

bool RedisDatabase::lset(const std::string& key, int index, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it == list_store.end()) return false;

    auto& lst = it->second;
    int size = static_cast<int>(lst.size());
    if (index < 0) index += size;

    if (index >= 0 && index < size) {
        lst[index] = value;
        return true;
    }
    return false;
}

// --- Persistence ---

bool RedisDatabase::dump(const std::string& filename) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ofstream ofs(filename);
    if (!ofs) return false;

    for (const auto& kv : kv_store) 
        ofs << "K " << kv.first << "\n" << kv.second << "\n";

    for (const auto& lkv : list_store) {
        ofs << "L " << lkv.first << "\n";
        for (const auto& item : lkv.second) ofs << item << "\n";
        ofs << "END_OF_LIST\n";
    }

    for (const auto& hkv : hash_store) {
        ofs << "H " << hkv.first << "\n";
        for (const auto& fkv : hkv.second) ofs << fkv.first << "\n" << fkv.second << "\n";
        ofs << "END_OF_HASH\n";
    }
    return true;
}

bool RedisDatabase::load(const std::string& filename) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ifstream ifs(filename);
    if (!ifs) return false;

    kv_store.clear();
    list_store.clear();
    hash_store.clear();

    std::string line;
    while (ifs >> line) { // Read the prefix (K, L, or H)
        char type = line[0];
        std::string key;
        ifs >> key;
        ifs.ignore(); // Clear newline after key

        if (type == 'K') {
            std::string value;
            std::getline(ifs, value);
            kv_store[key] = value;
        } else if (type == 'L') {
            while (std::getline(ifs, line) && line != "END_OF_LIST") {
                list_store[key].push_back(line);
            }
        } else if (type == 'H') {
            while (std::getline(ifs, line) && line != "END_OF_HASH") {
                std::string field = line;
                std::string value;
                std::getline(ifs, value);
                hash_store[key][field] = value;
            }
        }
    }
    return true;
}