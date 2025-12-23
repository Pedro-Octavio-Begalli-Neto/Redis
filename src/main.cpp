#include "../include/RedisServer.h"
#include "../include/RedisDatabase.h"
#include <iostream>
#include <thread>   // Necessary for std::thread
#include <chrono>   // Necessary for std::chrono

int main() {
    std::cout << "Iniciando processo do servidor..." << std::endl;

    // 1. Start the Background Persistence Thread FIRST
    std::thread persistThread([]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(300));
            // Using the Singleton to dump data
            if (RedisDatabase::getInstance().dump("dump.rdb")) {
                std::cout << "[LOG] Banco de dados persistido com sucesso." << std::endl;
            } else {
                std::cerr << "[ERRO] Falha ao persistir o banco de dados." << std::endl;
            }
        }
    });

    // 2. Detach or manage the thread
    // Detaching lets it run independently of the main thread's flow
    persistThread.detach();

    // 3. Start the Server (This is a blocking call)
    RedisServer server(6379);
    server.run(); 

    return 0; 
}

