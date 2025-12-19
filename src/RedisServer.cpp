#include "include/RedisServer.h"
#include <iostream>
#include <unistd.h>
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN // Avoids conflicts with old headers.
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")

#endif

static RedisServer* globalServer = nullptr;

RedisServer::RedisServer(int port) : port_(port), server_socket(-1), running(true) {
    
    globalServer = this;
}

void RedisServer::shutdown() {
    running = false;
    if (server_socket != -1) {
        close(server_socket);
        
    }
    std::cout << "Server shutdown completed!\n";}

void RedisServer::run() {
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    #ifdef _WIN32
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket. Error: " << WSAGetLastError() << "\n";
        return;
    }
    #endif

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Bind failed!\n";
        return;
    }

    if (listen(server_socket, 10) < 0) {
        std::cerr << "Listen failed!\n";
        return;
    }

    std::cout << "RedisServer is running on port " << port_ << "\n";
}
