#include "../include/RedisServer.h"
#include <iostream>

int main() {
    std::cout << "Iniciando processo do servidor..." << std::endl;
    RedisServer server(6379);
    server.run();
    return 0;
}

