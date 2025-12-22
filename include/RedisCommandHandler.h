#ifndef REDIS_COMMAND_HANDLER_H
#define REDIS_COMMAND_HANDLER_H

#include <string>

class RedisCommandHandler {
public:
    RedisCommandHandler();
    //Receive and process a command, returning the response as a string
    std::string handleCommand(const std::string& commandLine);
};
#endif // REDIS_COMMAND_HANDLER_H