#include <iostream>

int main(int argc, char *argv[]) {
    int port = 6379; // Default
    if (argc > 2) port = std::stoi(argv[1]);
    return 0;
    
}

