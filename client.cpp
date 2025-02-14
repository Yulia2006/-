#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080

std::string key;

std::string xor_encrypt_decrypt(const std::string& text) {
    std::string result = text;
    for (size_t i = 0; i < text.length(); i++) {
        result[i] ^= key[i % key.length()];
    }
    return result;
}

void receive_messages(SOCKET client_socket) {
    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            std::cerr << "Connection lost." << std::endl;
            break;
        }

        std::string decrypted_message = xor_encrypt_decrypt(std::string(buffer, bytes_received));
        std::cout << "\nMessage from the other user: " << decrypted_message << std::endl;
    }

    closesocket(client_socket);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock initialization failed!" << std::endl;
        return 1;
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    InetPton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to the server!" << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }


    char buffer[1024];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        std::cerr << "Error receiving message." << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }
    buffer[bytes_received] = '\0';
    std::cout << buffer << std::endl;

    std::cin >> key;
    std::cin.ignore();

    if (key.empty()) {
        std::cerr << "Key cannot be empty!" << std::endl;
        return 1;
    }


    if (send(client_socket, key.c_str(), key.length(), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send key!" << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        std::cerr << "Error receiving message." << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }
    buffer[bytes_received] = '\0';
    std::cout << "Server response: " << buffer << std::endl;

    std::thread receiver(receive_messages, client_socket);

    while (true) {
        std::string message;
        std::getline(std::cin, message);

        if (message.empty()) {
            std::cerr << "Message cannot be empty!" << std::endl;
            continue;
        }

        std::string encrypted_message = xor_encrypt_decrypt(message);

        if (send(client_socket, encrypted_message.c_str(), encrypted_message.length(), 0) == -1) {
            std::cerr << "Error sending message!" << std::endl;
            break;
        }
    }

    receiver.join();
    closesocket(client_socket);
    WSACleanup();
    return 0;
}
