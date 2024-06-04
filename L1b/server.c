#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
   int sockfd, newsockfd, portno;
   socklen_t clilen;
   char buffer[1024];
   struct sockaddr_storage cli_addr;
   struct addrinfo hints, *res;

   if (argc < 3) {
      fprintf(stderr, "ERROR, no IP address and port provided\n");
      exit(1);
   }

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;

   if (getaddrinfo(argv[1], argv[2], &hints, &res) != 0) {
      fprintf(stderr, "Error getting address info\n");
      exit(1);
   }

   sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

   if (sockfd < 0)
      error("ERROR opening socket");

   if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0)
      error("ERROR on binding");

   freeaddrinfo(res); 

   listen(sockfd, 5);

   clilen = sizeof(cli_addr);
   newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

   if (newsockfd < 0)
      error("ERROR on accept");

   bzero(buffer, 1024);
   ssize_t n = read(newsockfd, buffer, sizeof(buffer) - 1);

   if (n < 0) error("ERROR reading from socket");

   printf("The result: %s\n", buffer);

   char answer[1024];
   char ch;

   size_t length = strlen(buffer);

   for (size_t j = 0; j < length; j++) {
      ch = buffer[j];
      answer[j] = toupper(ch);
   }

   answer[length] = '\0';

   n = write(newsockfd, answer, strlen(answer));

   if (n < 0) error("ERROR writing to socket");

   close(newsockfd);
   close(sockfd);

   return 0;
}
