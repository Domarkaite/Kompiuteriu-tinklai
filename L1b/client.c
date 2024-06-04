#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    char buffer[1024];

    if (argc < 3) {
       fprintf(stderr, "usage %s IP_address port\n", argv[0]);
       exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

if (sockfd < 0) 
    error("ERROR opening socket");

bzero((char *) &serv_addr, sizeof(serv_addr));
portno = atoi(argv[2]);

if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
    // If not a valid IPv4 address, try parsing as IPv6
    sockfd = socket(AF_INET6, SOCK_STREAM, 0);

    if (sockfd < 0) 
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET6;

    if (inet_pton(AF_INET6, argv[1], &serv_addr.sin6_addr) <= 0) {
        fprintf(stderr, "Invalid IP address\n");
        exit(1);
    }
} else {
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
}

    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    printf("Please enter the message: ");
    bzero(buffer, 1024);
    fgets(buffer, 1023, stdin);
    n = write(sockfd, buffer, strlen(buffer));

    if (n < 0) 
         error("ERROR writing to socket");
    bzero(buffer, 1024);

    n = read(sockfd, buffer, 1023);
    if (n < 0) 
         error("ERROR reading from socket");

    printf("The received message: %s\n", buffer);

    close(sockfd);

    return 0;
}
