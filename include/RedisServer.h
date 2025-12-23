#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

#include <atomic>
#include <string>
#include <vector>
#include <thread>

// Configuração de tipos nativos para Windows e Linux
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET SocketType; // Define como unsigned long long no Win64
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    typedef int SocketType;
    #ifndef INVALID_SOCKET
        #define INVALID_SOCKET -1
    #endif
    #ifndef SOCKET_ERROR
        #define SOCKET_ERROR -1
    #endif
#endif

class RedisServer {
public:
    RedisServer(int port);
    void run();
    void shutdown();
    
private:
    int port_;
    SocketType server_socket; // Uso do tipo correto para evitar o Warning
    std::atomic<bool> running;
    std::vector<std::thread> threads;
    void setupSignalHandlers();


};

#endif