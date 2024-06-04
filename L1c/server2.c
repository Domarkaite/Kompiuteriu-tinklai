#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 22223
#define MAX_CLIENTS 10
#define MAX_NAME_LENGTH 20
#define MAX_MESSAGE_LENGTH 256

struct Client {
    int socket;
    char name[MAX_NAME_LENGTH];
};

struct ConnectToServerArgs {
    int port;
    int clientPort;
};

struct Client clients[MAX_CLIENTS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *handleClient(void *arg);
void *connectToServer();
void *handleClient2(void *arg);

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in6 serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    if ((serverSocket = socket(AF_INET6, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_port = htons(PORT);
    serverAddr.sin6_addr = in6addr_any;

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Binding error");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, MAX_CLIENTS) == -1) {
        perror("Listening error");
        exit(EXIT_FAILURE);
    }

    printf("Server is running...\n");

    pthread_t thread1, thread2;
    struct ConnectToServerArgs args1 = {22222, 22226};
    sleep(20);
    pthread_create(&thread1, NULL, connectToServer, (void *)&args1);
    pthread_detach(thread2);

    while (1) {
       if ((clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen)) == -1) {
            perror("Accepting error");
            continue;
        }
        if (ntohs(clientAddr.sin6_port) == 22228)
        {
            pthread_t thread;
            pthread_create(&thread, NULL, handleClient2, (void *)&clientSocket);
            pthread_detach(thread);
        }
        else
        {
            pthread_t thread;
            pthread_create(&thread, NULL, handleClient, (void *)&clientSocket);
            pthread_detach(thread);
        }
    }

    close(serverSocket);
    return 0;
}
void *handleClient2(void *arg) {
    int clientSocket = *((int *)arg);
    struct Client newClient;
    newClient.socket = clientSocket;

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == 0) {
            clients[i] = newClient;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
    while (1) {
        char message[MAX_MESSAGE_LENGTH];
        memset(message, 0, sizeof(message));
        if (recv(clientSocket, message, MAX_MESSAGE_LENGTH, 0) <= 0) {
            perror("Receiving message error");
            close(clientSocket);

            pthread_mutex_lock(&mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket == clientSocket) {
                    clients[i].socket = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);

            pthread_exit(NULL);
        }
        char *prefix = "PRANESIMAS";
        char *suffix = ": @4all";
        char *prefixPtr = strstr(message, prefix);
        if (prefixPtr == message) 
        { 
            char *suffixPtr = strstr(message, suffix);
            if (suffixPtr != NULL) {
                pthread_mutex_lock(&mutex);
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].socket != 0 && clients[i].socket != clientSocket) {
                        char broadcastMessage[MAX_MESSAGE_LENGTH + MAX_NAME_LENGTH + 30];
                        snprintf(broadcastMessage, sizeof(broadcastMessage), "%s\n\r", message);

                        int sentBytes = send(clients[i].socket, broadcastMessage, strlen(broadcastMessage), 0);
                        if (sentBytes < 0) {
                            perror("Sending message error");
                        }
                    }
                }
                pthread_mutex_unlock(&mutex);
            } 
        } 
    }
}
void *handleClient(void *arg) {
    int clientSocket = *((int *)arg);
    char name[MAX_NAME_LENGTH];

    send(clientSocket, "ATSIUSKVARDA\n\r", strlen("ATSIUSKVARDA\n\r"), 0);

    if (recv(clientSocket, name, MAX_NAME_LENGTH, 0) <= 0) {
        perror("Receiving name error");
        close(clientSocket);
        pthread_exit(NULL);
    }

    name[strcspn(name, "\n\r")] = '\0';

    struct Client newClient;
    newClient.socket = clientSocket;
    strncpy(newClient.name, name, MAX_NAME_LENGTH);

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == 0) {
            clients[i] = newClient;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    send(clientSocket, "VARDASOK\n\r", strlen("VARDASOK\n\r"), 0);

    while (1) {
        char message[MAX_MESSAGE_LENGTH];
        memset(message, 0, sizeof(message)); 
        if (recv(clientSocket, message, MAX_MESSAGE_LENGTH, 0) <= 0) {
            perror("Receiving message error");
            close(clientSocket);

            pthread_mutex_lock(&mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket == clientSocket) {
                    clients[i].socket = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);

            pthread_exit(NULL);
        }
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket != 0) {
                char broadcastMessage[MAX_MESSAGE_LENGTH + MAX_NAME_LENGTH + 30];
                snprintf(broadcastMessage, sizeof(broadcastMessage), "PRANESIMAS %s: %s\n\r", newClient.name, message);

                int sentBytes = send(clients[i].socket, broadcastMessage, strlen(broadcastMessage), 0);
                if (sentBytes < 0) {
                    perror("Sending message error");
                }
            }
        }
        pthread_mutex_unlock(&mutex);
    }
}

void *connectToServer(void *arg) {
    int clientSocket;
    struct sockaddr_in6 server1Addr;
    struct ConnectToServerArgs *args = (struct ConnectToServerArgs *)arg;

    clientSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server1Addr, 0, sizeof(server1Addr));
    server1Addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &server1Addr.sin6_addr);
    server1Addr.sin6_port = htons(args->port); 

    struct Client newClient;
    newClient.socket = clientSocket;

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == 0) {
            clients[i] = newClient;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    struct sockaddr_in6 clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin6_family = AF_INET6;
    clientAddr.sin6_addr = in6addr_any;
    clientAddr.sin6_port = htons(args->clientPort);

    if (bind(clientSocket, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) == -1) {
        perror("Binding failed");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    if (connect(clientSocket, (struct sockaddr *)&server1Addr, sizeof(server1Addr)) == -1) {
        fprintf(stderr, "Error connecting to Server: %s\n", strerror(errno));
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server as client.\n");

    while (1) {
        char message[MAX_MESSAGE_LENGTH];
        memset(message, 0, sizeof(message)); 
        if (recv(clientSocket, message, MAX_MESSAGE_LENGTH, 0) <= 0) {
            
            perror("Receiving message error");
            close(clientSocket);

            pthread_mutex_lock(&mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket == clientSocket) {
                    clients[i].socket = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);

            pthread_exit(NULL);
        }
        char *prefix = "PRANESIMAS";
        char *suffix = ": @4all";
        char *prefixPtr = strstr(message, prefix);
        if (prefixPtr == message) 
        { 
            char *suffixPtr = strstr(message, suffix);
            if (suffixPtr != NULL) {
                pthread_mutex_lock(&mutex);
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].socket != 0 && clients[i].socket != clientSocket) {
                        char broadcastMessage[MAX_MESSAGE_LENGTH + MAX_NAME_LENGTH + 30];
                        snprintf(broadcastMessage, sizeof(broadcastMessage), "%s\n\r", message);

                        int sentBytes = send(clients[i].socket, broadcastMessage, strlen(broadcastMessage), 0);
                        if (sentBytes < 0) {
                            perror("Sending message error");
                        }
                    }
                }
                pthread_mutex_unlock(&mutex);
            } 
        } 
    }

    close(clientSocket);
    pthread_exit(NULL);
}