#include "../include/RedisServer.h"
#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <signal.h>
#include <vector>

#ifdef _WIN32
    #define closesocket_func closesocket
#else
    #define closesocket_func close
#endif

static RedisServer* global_server = nullptr;

void signalHandler(int signum) {
    if (global_server) {
        std::cout << "Shutting down server..." << std::endl;
        global_server->shutdown();
    }
    exit(signum);
}

void RedisServer::setupSignalHandlers() {
    #ifdef _WIN32
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
    #else
        struct sigaction action{};
        action.sa_handler = signalHandler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;

        sigaction(SIGINT, &action, nullptr);
        sigaction(SIGTERM, &action, nullptr);
    #endif
}

RedisServer::RedisServer(int port) : port_(port), server_socket(INVALID_SOCKET), running(true) {
    global_server = this;
    setupSignalHandlers();
}

void RedisServer::shutdown() {
    running = false;
    if (server_socket != INVALID_SOCKET) closesocket_func(server_socket);
}

void RedisServer::run() {
    #ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif

    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "ERRO: Porta ocupada!" << std::endl;
        return;
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    if (RedisDatabase::getInstance().load("dump.rdb")) {
        std::cout << "[LOG] Banco de dados carregado com sucesso." << std::endl;
    } else {
        std::cout << "[ERRO] Falha ao carregar o banco de dados." << std::endl;
    }

    listen(server_socket, 10);
    std::cout << "Redis Server Listening On Port " << port_ << std::endl;

    RedisCommandHandler handler;

    while (running) {
        SOCKET client = accept(server_socket, nullptr, nullptr);
        if (client == INVALID_SOCKET) break;

        std::thread([client, &handler]() {
            char buffer[1024];
            while (true) {
                memset(buffer, 0, 1024);
                int bytes = recv(client, buffer, 1023, 0);
                if (bytes <= 0) break;

                std::string req(buffer, bytes);
                std::cout << "[LOG] Recebido: " << req << std::endl;

                std::string res = handler.handleCommand(req);
                send(client, res.c_str(), (int)res.size(), 0);
            }
            closesocket(client);
        }).detach();
    }
}