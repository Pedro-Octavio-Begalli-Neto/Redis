#include <iostream>
#include <string>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(6379);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Erro: Certifique-se de que o SERVIDOR esta rodando primeiro!" << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to Redis at 127.0.0.1:6379" << std::endl;

    char buffer[1024];
    std::string input;
    while (true) {
        std::cout << "127.0.0.1:6379> ";
        if (!std::getline(std::cin, input) || input == "exit") break;

        send(client_socket, input.c_str(), (int)input.length(), 0);

        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << buffer << std::endl;
        }
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}