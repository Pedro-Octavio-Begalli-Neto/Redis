#ifndef REDIS_DATABASE_H  // Note the 'n' for "if not defined"
#define REDIS_DATABASE_H
#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <chrono>


class RedisDatabase{
private:
    RedisDatabase() = default;
    ~RedisDatabase() = default;
    RedisDatabase(const RedisDatabase&) = delete;
    RedisDatabase& operator=(const RedisDatabase&) = delete;

    std::mutex db_mutex;
    std::unordered_map<std::string, std::string> kv_store;
    std::unordered_map<std::string, std::vector<std::string>> list_store;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hash_store;
    std::unordered_map<std::string, std::string> expire_store;

    std::unordered_map<std::string, time_t> expire_times;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> expire_clocks;


public:
    static RedisDatabase& getInstance();
    bool dump(const std::string& filename);
    bool load(const std::string& filename);

    bool flushAll() {
        std::lock_guard<std::mutex> lock(db_mutex);
        kv_store.clear();
        list_store.clear();
        hash_store.clear();
        return true;
    }


    void set (const std::string& key, const std::string& value);
    bool get (const std::string& key, std::string& value);
    std::vector<std::string> lget(const std::string& key);
    void lset(const std::string& key, const std::vector<std::string>& value);
    std::unordered_map<std::string, std::string> hget(const std::string& key);
    void hset(const std::string& key, const std::unordered_map<std::string, std::string>& value);


    std::vector<std::string> keys();
    bool del(const std::string& key);
    bool expire(const std::string& key, int seconds);
    bool rename(const std::string& old_key, const std::string& new_key);
    bool expiretime(const std::string& key, time_t& expire_time);
    std::string type(const std::string& key);
    ssize_t llen(const std::string& key);
    void lpush(const std::string& key, const std::string& value);
    void rpush(const std::string& key, const std::string& value);
    bool lpop(const std::string& key, std::string& value);
    bool rpop(const std::string& key, std::string& value);
    int lrem(const std::string& key, const std::string& value, int count);
    bool lindex(const std::string& key, int index, std::string& value);
    bool lset(const std::string& key, int index, const std::string& value);

    bool dump(const std::string& key, std::string& serialized_value);
    bool restore(const std::string& key, const std::string& serialized_value, int ttl);
    bool load(const std::string& filename, bool replace);
};





#endif