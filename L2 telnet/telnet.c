#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SZ 1024
#define DEBUG 0

#define IAC 255
#define DONT 254
#define DO 253
#define WONT 252
#define WILL 251
#define SE 240
#define SB 250

void handle_Server_Data(int sck_fd, char *data, int num_bytes)
{
	char out_buf[BUFFER_SZ];
	int out_ptr = 0;

	for(int i = 0; i < num_bytes; i++)
	{
		if(data[i] == (char)IAC)
		{
			out_buf[out_ptr++] = IAC;

			i++;
			switch(data[i])
			{
				case ((char)WILL):
					if(DEBUG){ printf("WILL\n"); }
					out_buf[out_ptr++] = (char)DONT;
					i++;
					out_buf[out_ptr++] = data[i];
					break;
				case ((char)WONT):
					if(DEBUG){ printf("Got WONT\n"); }
					break;
				case ((char)DO):
					if(DEBUG){ printf("DO\n"); }
					out_buf[out_ptr++] = (char)(char)WONT;
					i++;
					out_buf[out_ptr++] = data[i];
					break;
				case ((char)DONT):
					if(DEBUG){ printf("DONT\n"); }
					break;
				default:
					if(DEBUG){ printf("Not yet implemented %x\n", (int)data[i]); }
					break;
			}
		}else{
			//is data
			printf("%c", data[i]);
		}
	}

	fflush(stdout); 
	
	if(write(sck_fd, out_buf, out_ptr) == -1){
		printf("error write()'ing\n");
		exit(0);
	}

	if(DEBUG){ printf("packet sent!\n\n"); }
}

int maximum(int a, int b){
	return a > b ? a : b;
}


void start_communication(int sck_fd){
	char inp_buf[BUFFER_SZ];
	char *user_command = NULL;
	size_t user_max_length = 0;
	
	fd_set fds;
	FD_ZERO(&fds);
	int mx;
	while(1){

		FD_SET(fileno(stdin), &fds);
		FD_SET(sck_fd, &fds);
		mx = maximum(fileno(stdin) + 1, sck_fd + 1);

		select(mx, &fds, NULL, NULL, NULL); //IO Multiplexing

		if(FD_ISSET(fileno(stdin), &fds)){
			if(DEBUG){ printf("reading from stdin\n"); }
			int num_read = getline(&user_command, &user_max_length, stdin);
			if(num_read == -1){
				printf("error getline()'ing\n");
				exit(0);
			}
			write(sck_fd, user_command, num_read);
		}

		if(FD_ISSET(sck_fd, &fds)){
			int num_read = read(sck_fd, inp_buf, BUFFER_SZ);
			if(DEBUG){ printf("reading from socket(%d bytes read)\n", num_read); }
			if(num_read == -1){
				printf("error read()\n");
				exit(0);
			}else if(num_read == 0){
				printf("Connection closed\n");
				exit(0);
			}
			handle_Server_Data(sck_fd, inp_buf, num_read);
		}
	}
}

int main(int argc, char *argv[]){

	if(argc != 3){
		printf("Usage: ./a.out <ip_address> <port_no>\n");
		return 0;
	}

	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	struct addrinfo *results;

	if(getaddrinfo(argv[1], argv[2], &hints, &results) != 0){
		printf("error getaddrinfo()\n");
		return 0;
	}

	struct addrinfo *addr_okay;
	int succeeded = 0;
	int sck_fd = socket(AF_INET, SOCK_STREAM, 0);
	for(addr_okay = results; addr_okay != NULL; addr_okay = addr_okay->ai_next){
		if(connect(sck_fd, addr_okay->ai_addr, addr_okay->ai_addrlen) >= 0){
			succeeded = 1;
			break;
		}
	}
	freeaddrinfo(results);

	if(!succeeded){
		printf("error connect()\n");
		return 0;
	}

	start_communication(sck_fd);
	close(sck_fd);

	return 0;
}