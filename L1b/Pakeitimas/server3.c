#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>


#define SERVER3_PORT 10003
#define SERVER2_PORT 10002
#define SERVER22_PORT 10005
#define SERVER33_PORT 10006

void forwardToClient2(int clientSocket, char *message) {
    send(clientSocket, message, strlen(message), 0);
}

void forwardMessageToServer2(char *message) {
    int clientSocket;
    struct sockaddr_in6 server2Addr;

    clientSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server2Addr, 0, sizeof(server2Addr));
    server2Addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &server2Addr.sin6_addr);
    server2Addr.sin6_port = htons(SERVER2_PORT);

    struct sockaddr_in6 clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin6_family = AF_INET6;
    clientAddr.sin6_addr = in6addr_any;
    clientAddr.sin6_port = htons(10006);

    if (bind(clientSocket, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) == -1) {
        perror("Binding failed");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    if (connect(clientSocket, (struct sockaddr *)&server2Addr, sizeof(server2Addr)) == -1) {
        fprintf(stderr, "Error connecting to Server 1: %s\n", strerror(errno));
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    send(clientSocket, message, strlen(message), 0);


    close(clientSocket);
}

void *handleClient2(void *arg) {
    int clientSocket = *((int *)arg);
    char buffer[1024];
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        printf("Server 3 received from Client 2: %s\n", buffer);

        if (ntohs(((struct sockaddr_in6 *)arg)->sin6_port) == SERVER22_PORT) {
            forwardToClient2(clientSocket, buffer);
        } else {
            forwardMessageToServer2(buffer);
        }
    }

    close(clientSocket);
    pthread_exit(NULL);
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
    serverAddr.sin6_port = htons(SERVER3_PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Binding failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("Server 3 is listening on port %d\n", SERVER3_PORT);

    if (listen(serverSocket, 5) == -1) {
        perror("Listening failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("Server 3 is waiting for connections...\n");

    while (1) {
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);

        if (clientSocket != -1) {
            printf("Accepted the connection from Client 2\n");

            pthread_t tid;
            int *clientSockPtr = malloc(sizeof(int));
            *clientSockPtr = clientSocket;
            pthread_create(&tid, NULL, handleClient2, (void *)clientSockPtr);
            pthread_detach(tid);
        }

        int server2Socket = accept(serverSocket, NULL, NULL);
        if (server2Socket != -1) {
            printf("Accepted the connection from Server 2\n");
            struct sockaddr_in6 peerAddr;
            socklen_t peerLen = sizeof(peerAddr);
            getpeername(server2Socket, (struct sockaddr *)&peerAddr, &peerLen);
            if (ntohs(peerAddr.sin6_port) == 10005){
            char buffer[1024];
            ssize_t bytesRead = recv(server2Socket, buffer, sizeof(buffer), 0);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0'; 
                printf("Server 3 received from Server 2: %s\n", buffer);

                forwardToClient2(clientSocket, buffer);
            }
            close(server2Socket);}
            else
            {
                printf("Accepted the connection from Client 2\n");

            pthread_t tid;
            int *clientSockPtr = malloc(sizeof(int));
            *clientSockPtr = clientSocket;
            pthread_create(&tid, NULL, handleClient2, (void *)clientSockPtr);
            pthread_detach(tid);
            }
        }
    }

    close(serverSocket);

    return 0;
}
