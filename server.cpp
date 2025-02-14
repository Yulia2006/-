#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080

std::vector<SOCKET> clients;
std::mutex clients_mutex;
std::string keys[2];

void handle_client(SOCKET client_socket, SOCKET other_client_socket, int client_index) {
    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        
        if (bytes_received <= 0) {
            std::cerr << "Client disconnected." << std::endl;
            break;
        }

        std::cout << "Received encrypted message: " << buffer << std::endl;
        
        if (send(other_client_socket, buffer, bytes_received, 0) == -1) {
            std::cerr << "Error sending message." << std::endl;
            break;
        }
    }

    closesocket(client_socket);
    keys[client_index].clear();
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock initialization failed!" << std::endl;
        return 1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Socket binding failed!" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, 2) == SOCKET_ERROR) {
        std::cerr << "Listening failed!" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server started. Waiting for connections..." << std::endl;

    while (clients.size() < 2) {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Connection acceptance failed!" << std::endl;
            continue;
        }

        std::lock_guard<std::mutex> lock(clients_mutex);
        
        if (clients.size() == 2) {
            closesocket(client_socket);
            continue;
        }

        clients.push_back(client_socket);
        std::cout << "Client connected. Total clients: " << clients.size() << "\n";
        if (clients.size() == 2) {
            for (int i = 0; i < 2; i++) {
                char buffer[1024];
                const char* request_key_msg = "Please enter your encryption key: ";
                send(clients[i], request_key_msg, strlen(request_key_msg), 0);

                int bytes_received = recv(clients[i], buffer, sizeof(buffer), 0);
                
                if (bytes_received <= 0) {
                    std::cerr << "Error receiving encryption key." << std::endl;
                    continue;
                }
                
                buffer[bytes_received] = '\0';
                if (strlen(buffer) == 0) {
                    const char* error_message = "Key cannot be empty!";
                    send(clients[i], error_message, strlen(error_message), 0);
                    closesocket(clients[i]);
                    continue;
                }

                keys[i] = std::string(buffer);
                std::cout << "Client " << i + 1 << " key received: " << keys[i] << std::endl;
            }

            if (keys[0] != keys[1]) {
                const char* error_message = "Keys do not match. Disconnecting...";
                send(clients[0], error_message, strlen(error_message), 0);
                send(clients[1], error_message, strlen(error_message), 0);
                closesocket(clients[0]);
                closesocket(clients[1]);
                break;
            }
            
            const char* success_message = "Keys match. You can start sending messages.";
            send(clients[0], success_message, strlen(success_message), 0);
            send(clients[1], success_message, strlen(success_message), 0);

            std::cout << "Both clients have the same key. Proceeding with communication..." << std::endl;
            
            std::thread t1(handle_client, clients[0], clients[1], 0);
            std::thread t2(handle_client, clients[1], clients[0], 1);

            t1.join();
            t2.join();
        }
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
