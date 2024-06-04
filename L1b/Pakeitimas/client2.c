#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER3_PORT 10003

void receiveFromServer3(int clientSocket) {
    char buffer[1024];
    int bytesRead;

    bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead <= 0) {
        // Handle connection closure or error
        perror("Error receiving from Server 3");
        return;
    }

    buffer[bytesRead] = '\0';
    printf("Received from Server 3: %s", buffer);
}

void forwardMessageToServer3(int clientSocket, char *message) {
    send(clientSocket, message, strlen(message), 0);
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

    // Set up server3 address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &serverAddr.sin6_addr);
    serverAddr.sin6_port = htons(SERVER3_PORT);

    // Connect to Server 3
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Connection to Server 3 failed");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    // User choice
    int choice;
    printf("Choose an option:\n1. Send message to Server 3\n2. Receive message from Server 3\n");
    scanf("%d", &choice);
    getchar();  // Consume the newline character

    if (choice == 1) {
        // Send a message to Server 3
        char message[1024];
        printf("Enter message for Server 3: ");
        fgets(message, sizeof(message), stdin);
        forwardMessageToServer3(clientSocket, message);
    } else if (choice == 2) {
        // Receive a message from Server 3
        receiveFromServer3(clientSocket);
    } else {
        printf("Invalid choice\n");
    }

    // Close client socket
    close(clientSocket);

    return 0;
}
