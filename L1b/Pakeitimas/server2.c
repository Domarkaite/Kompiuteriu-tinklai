#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>


#define SERVER1_PORT 10001
#define SERVER11_PORT 10004
#define SERVER2_PORT 10002
#define SERVER3_PORT 10003
#define SERVER33_PORT 10006

void forwardMessageToServer1(char *message) {
    int clientSocket;
    struct sockaddr_in6 server1Addr;

    clientSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server1Addr, 0, sizeof(server1Addr));
    server1Addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &server1Addr.sin6_addr);
    server1Addr.sin6_port = htons(SERVER1_PORT); 
  
    struct sockaddr_in6 clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin6_family = AF_INET6;
    clientAddr.sin6_addr = in6addr_any;
    clientAddr.sin6_port = htons(10005);

    if (bind(clientSocket, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) == -1) {
        perror("Binding failed");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    if (connect(clientSocket, (struct sockaddr *)&server1Addr, sizeof(server1Addr)) == -1) {
        fprintf(stderr, "Error connecting to Server 1: %s\n", strerror(errno));
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    send(clientSocket, message, strlen(message), 0);


    close(clientSocket);
}




void forwardMessageToServer3(char *message) {
    int clientSocket;
    struct sockaddr_in6 server3Addr;

    clientSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server3Addr, 0, sizeof(server3Addr));
    server3Addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &server3Addr.sin6_addr);
    server3Addr.sin6_port = htons(SERVER3_PORT);

    struct sockaddr_in6 clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin6_family = AF_INET6;
    clientAddr.sin6_addr = in6addr_any;
    clientAddr.sin6_port = htons(10005);

    if (bind(clientSocket, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) == -1) {
        perror("Binding failed");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    if (connect(clientSocket, (struct sockaddr *)&server3Addr, sizeof(server3Addr)) == -1) {
        perror("Connection to Server 3 failed");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    send(clientSocket, message, strlen(message), 0);

    close(clientSocket);
}



void handleClient(int clientSocket, struct sockaddr_in6 clientAddr) {
    char buffer[1024];
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead <= 0) {
        perror("Error reading from client");
    } else {
        buffer[bytesRead] = '\0'; 

        if (ntohs(clientAddr.sin6_port) == SERVER33_PORT) {
            printf("Server 2 received: %s\n", buffer);
            forwardMessageToServer1(buffer);
        } else if (ntohs(clientAddr.sin6_port) == SERVER11_PORT) {
            printf("Server 2 received: %s\n", buffer);
            forwardMessageToServer3(buffer);
        }
        close(clientSocket);
    }
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in6 serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    // Create socket
    serverSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_addr = in6addr_any;
    serverAddr.sin6_port = htons(SERVER2_PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Binding failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 1) == -1) {
        perror("Listening failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
    if (clientSocket == -1) {
        perror("Acceptance failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    handleClient(clientSocket, clientAddr);

    close(serverSocket);

    return 0;
}