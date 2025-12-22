#include "../include/RedisServer.h"
#include "../include/RedisCommandHandler.h"
#include <iostream>
#include <cstring>
#include <thread>

#ifdef _WIN32
    #define closesocket_func closesocket
#else
    #define closesocket_func close
#endif

RedisServer::RedisServer(int port) : port_(port), server_socket(INVALID_SOCKET), running(true) {}

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