#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 22222
#define MAX_CLIENTS 10
#define MAX_NAME_LENGTH 20
#define MAX_MESSAGE_LENGTH 256

struct Client {
    int socket;
    char name[MAX_NAME_LENGTH];
};

struct Client clients[MAX_CLIENTS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *handleClient(void *arg);
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

    while (1) {
        if ((clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen)) == -1) {
            perror("Accepting error");
            continue;
        }
        if (ntohs(clientAddr.sin6_port) == 22226)
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