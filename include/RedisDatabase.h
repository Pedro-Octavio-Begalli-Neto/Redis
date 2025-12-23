#ifndef REDIS_DATABASE_H  // Note the 'n' for "if not defined"
#define REDIS_DATABASE_H
#include <string>


class RedisDatabase{
private:
    RedisDatabase() = default;
    ~RedisDatabase() = default;
    RedisDatabase(const RedisDatabase&) = delete;
    RedisDatabase& operator=(const RedisDatabase&) = delete;

public:
    static RedisDatabase& getInstance();
    bool dump(const std::string& filename);
    bool load(const std::string& filename);
};



#endif