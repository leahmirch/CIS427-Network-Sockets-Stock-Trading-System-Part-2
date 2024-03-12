#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "database.h"
#include "processCommand.cpp"
#include <atomic>

std::atomic<bool> shutdownRequested(false);

#define SERVER_PORT 2025
#define BUFFER_SIZE 1024

struct ClientThreadArgs {
    int clientSocket;
    Database* db;
    std::string clientIp;
};

// Function to handle client connections
void* handleClient(void* args) {
    auto* clientArgs = static_cast<ClientThreadArgs*>(args);
    int clientSocket = clientArgs->clientSocket;
    Database* db = clientArgs->db;

    char buffer[BUFFER_SIZE];
    bool isRunning = true;

    // Main loop for handling client commands
    while (isRunning && !shutdownRequested.load()) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);

        if (bytesReceived > 0) {
            std::string command(buffer, bytesReceived);
            std::cout << "- Received command: " << command << std::endl;

            // Process commands based on their type
            if (startsWith(command, "TEST")) {
                processTestCommand(clientSocket, command);
            } else if (startsWith(command, "LOGIN")) {
                processLoginCommand(db, clientSocket, command);
            } else if (startsWith(command, "LOGOUT")) {
                processLogoutCommand(db, clientSocket, command);
            } else if (startsWith(command, "WHO")) {
                processWhoCommand(db, clientSocket, clientArgs->clientIp);
            } else if (startsWith(command, "LIST")) {
                processListCommand(db, clientSocket, command);
            } else if (startsWith(command, "LOOKUP")) {
                processLookupCommand(db, clientSocket, command);
            } else if (startsWith(command, "BUY")) {
                processBuyCommand(db->getDb(), clientSocket, command);
            } else if (startsWith(command, "SELL")) {
                processSellCommand(db->getDb(), clientSocket, command);
            } else if (startsWith(command, "DEPOSIT")) {
                processDepositCommand(db, clientSocket, command);
            } else if (startsWith(command, "BALANCE")) {
                processBalanceCommand(db, clientSocket, command);
            } else if (startsWith(command, "SHUTDOWN")) {
                processShutdownCommand(db, clientSocket, command);
            } else if (startsWith(command, "QUIT")) {
                processQuitCommand(db, clientSocket, command);
            } else {
                std::cout << "400 invalid command.\n"; // Debugging line
                sendMessage(clientSocket, "400 invalid command\n"); // Send error message to client
            }
        } else {
            if (bytesReceived == 0) {
                std::cout << "Client disconnected." << std::endl;
            } else {
                std::cerr << "Received error." << std::endl;
            }
            isRunning = false;
        }
    }
    // Cleanup and close socket before exiting the thread
    std::cout << "Closing client socket due to shutdown request or client disconnect." << std::endl;
    close(clientSocket);
    delete clientArgs; // Clean up the allocated memory for thread arguments
    return nullptr;
}

int main() {
    Database db("stock_trading.db"); // Ensure the database filename is correct
    
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrSize;

    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Bind failed." << std::endl;
        return 1;
    }

    listen(serverSocket, 5);
    std::cout << "\nServer listening on port " << SERVER_PORT << "." << std::endl << std::endl;

    while (true) {
        addrSize = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrSize);
        if (clientSocket < 0) {
            std::cerr << "Accept failed." << std::endl;
            continue;
        }

        // Extract the client's IP address
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);

        std::cout << "Client connected from " << clientIp << std::endl;

        // Create thread arguments and start a new thread to handle the client
        ClientThreadArgs* args = new ClientThreadArgs{clientSocket, &db, std::string(clientIp)};
        pthread_t threadId;
        if (pthread_create(&threadId, nullptr, handleClient, args) < 0) {
            std::cerr << "Could not create thread." << std::endl;
            delete args; // Cleanup if thread creation fails
        }
        pthread_detach(threadId); // Detach the thread to avoid memory leaks
    }
    close(serverSocket);
    return 0;
}