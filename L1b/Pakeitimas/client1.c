#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER1_PORT 10001

void forwardMessageToServer1(int clientSocket, char *message) {
    send(clientSocket, message, strlen(message), 0);
}

void receiveFromServer1(int clientSocket) {
    char buffer[1024];
    int bytesRead;

    bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead <= 0) {
        // Handle connection closure or error
        perror("Error receiving from Server 1");
    } else {
        buffer[bytesRead] = '\0';
        printf("Received from Server 1: %s", buffer);
    }
}

int main() {
    int clientSocket;
    struct sockaddr_in6 serverAddr;

    // Create socket
    clientSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server1 address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &serverAddr.sin6_addr);
    serverAddr.sin6_port = htons(SERVER1_PORT);

    // Connect to Server 1
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Connection to Server 1 failed");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    // Menu for sending or receiving messages
    int choice;
    char message[1024];

    printf("1. Send message to Server 1\n");
    printf("2. Receive message from Server 1\n");
    printf("Enter your choice: ");
    scanf("%d", &choice);
    getchar();  // Consume the newline character

    switch (choice) {
        case 1:
            printf("Enter message for Server 1: ");
            fgets(message, sizeof(message), stdin);
            forwardMessageToServer1(clientSocket, message);
            break;

        case 2:
            receiveFromServer1(clientSocket);
            break;

        default:
            printf("Invalid choice\n");
    }

    // Close client socket
    close(clientSocket);

    return 0;
}
