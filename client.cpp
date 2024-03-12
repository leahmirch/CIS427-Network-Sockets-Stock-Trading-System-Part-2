#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define SERVER_PORT 2025

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <Server IP>" << std::endl;
        return 1;
    }

    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[1024];

    // Create socket
    clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);

    // Connect to server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection failed." << std::endl;
        return 1;
    }

    std::cout << std::endl << "Connected to the server." << std::endl;
    fd_set readfds;
    int max_sd = clientSocket;

    // Main loop for sending and receiving data
    while (true) {
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        std::cout << "-> "; // Print prompt before reading input
        std::flush(std::cout); // Ensure the prompt is displayed immediately

        // Wait for activity on socket descriptors
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            std::cerr << "Select error." << std::endl;
            break;
        }

        // Check for activity on stdin
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            std::string cmd;
            std::getline(std::cin, cmd);
            if (!cmd.empty()) {
                // Handle QUIT command
                if (cmd == "QUIT") {
                    send(clientSocket, "QUIT", strlen("QUIT"), 0);
                    char quitResponse[1024];
                    recv(clientSocket, quitResponse, sizeof(quitResponse), 0);
                    std::cout << "-> Server: 200 OK\n";
                    std::cout << "* Client quitting *\n";
                    std::cout << "Goodbye! \n" << std::endl;
                    break; // Exit loop if QUIT command is entered
                }
                // Send command to server
                send(clientSocket, cmd.c_str(), cmd.length(), 0);
            }
        }

        // Check for activity on clientSocket
        if (FD_ISSET(clientSocket, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived <= 0) {
                std::cout << "Server disconnected." << std::endl;
                break;
            } else {
                std::cout << "Server: " << buffer << std::endl;
            }
        }
    }

    // Cleanup and close socket
    close(clientSocket);
    return 0;
}