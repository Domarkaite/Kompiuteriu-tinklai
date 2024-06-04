#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define BUFFER_SZ 1024
#define MAX_SERVERS 100
#define MAX_ADDRESS_LENGTH 100
#define MAX_COMMANDS 100
#define MAX_COMMAND_LENGTH 100
#define DEBUG 0

#define IAC 255
#define DONT 254
#define DO 253
#define WONT 252
#define WILL 251
#define SE 240
#define SB 250

typedef struct {
    char address[MAX_ADDRESS_LENGTH];
    int port;
    char commands[MAX_COMMANDS][MAX_COMMAND_LENGTH];
    int commands_count;
} Server;
void print_current_commands(Server *server); 
int read_servers_from_file(const char *filename, Server servers[]) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file %s\n", filename);
        return 0;
    }

    int count = 0;
    char line[BUFFER_SZ];

    while (fgets(line, BUFFER_SZ, file) != NULL) {
        if (sscanf(line, "server: %99s %d", servers[count].address, &servers[count].port) == 2) {
            servers[count].commands_count = 0;

            while (fgets(line, BUFFER_SZ, file) != NULL && line[0] != '\n') {
                if (sscanf(line, "commands: %99[^\n]", servers[count].commands[servers[count].commands_count]) == 1) {
                    servers[count].commands_count++;
                    if (servers[count].commands_count >= MAX_COMMANDS) {
                        printf("Maximum number of commands reached for server %s\n", servers[count].address);
                        break;
                    }
                }
            }
            count++;
            if (count >= MAX_SERVERS) {
                printf("Maximum number of servers reached\n");
                break;
            }
        }
    }

    fclose(file);
    return count;
}

void write_servers_to_file(const char *filename, Server servers[], int count) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error opening file %s\n", filename);
        return;
    }

    for (int i = 0; i < count; i++) {
        fprintf(file, "server: %s %d\n", servers[i].address, servers[i].port);
        for (int j = 0; j < servers[i].commands_count; j++) {
            // Remove newline characters from commands
            size_t command_len = strcspn(servers[i].commands[j], "\n");
            fprintf(file, "commands: %.*s\n", (int)command_len, servers[i].commands[j]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
}





void print_servers(Server servers[], int count) {
    printf("Servers previously called:\n");
    for (int i = 0; i < count; i++) {
        printf("%d. %s:%d\n", i + 1, servers[i].address, servers[i].port);
    }
}

void show_commands_for_server(Server servers[], int count, int server_choice) 
{
    if (server_choice < 1 || server_choice > count) {
        printf("Invalid server choice\n");
        return;
    }

    Server selected_server = servers[server_choice - 1];
    printf("Commands used for server %s:%d:\n", selected_server.address, selected_server.port);
    for (int i = 0; i < selected_server.commands_count; i++) {
        printf("%s\n", selected_server.commands[i]);
    }
}

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
                    if(DEBUG)
                    { 
                        printf("WILL\n"); 
                    }
                    out_buf[out_ptr++] = (char)DONT;
                    i++;
                    out_buf[out_ptr++] = data[i];
                    break;
                case ((char)WONT):
                    if(DEBUG)
                    { 
                        printf("Got WONT\n"); 
                    }
                    break;
                case ((char)DO):
                    if(DEBUG)
                    { 
                        printf("DO\n"); 
                    }
                    out_buf[out_ptr++] = (char)WONT;
                    i++;
                    out_buf[out_ptr++] = data[i];
                    break;
                case ((char)DONT):
                    if(DEBUG)
                    { 
                        printf("DONT\n"); 
                    }
                    break;
                default:
                    if(DEBUG)
                    { 
                        printf("Not yet implemented %x\n", (int)data[i]);
                    }
                    break;
            }
        }
        else
        {
            printf("%c", data[i]);
        }
    }

    fflush(stdout); 
    
    if(write(sck_fd, out_buf, out_ptr) == -1)
    {
        printf("error write()\n");
        exit(0);
    }
}

int maximum(int a, int b){
    return a > b ? a : b;
}

void add_command_to_server(Server *server, const char *command) {
    if (server != NULL && command != NULL && server->commands_count < MAX_COMMANDS) {
        
        char sanitized_command[MAX_COMMAND_LENGTH];
        strncpy(sanitized_command, command, MAX_COMMAND_LENGTH - 1);
        sanitized_command[MAX_COMMAND_LENGTH - 1] = '\0';
        char *newline_pos = strchr(sanitized_command, '\n');
        if (newline_pos != NULL) {
            *newline_pos = '\0'; 
        }

        for (int i = 0; sanitized_command[i] != '\0'; i++) {
            if (!isspace((unsigned char)sanitized_command[i])) {
                strncpy(server->commands[server->commands_count], sanitized_command, MAX_COMMAND_LENGTH - 1);
                server->commands[server->commands_count][MAX_COMMAND_LENGTH - 1] = '\0';
                server->commands_count++;
                break;
            }
        }
    }
}

void start_communication(int sck_fd, Server *server, Server servers[], int count)
{
    char inp_buf[BUFFER_SZ];
    char *user_command = NULL;
    size_t user_max_length = 0;
    
    fd_set fds;
    FD_ZERO(&fds);
    int mx;
    int connection_closed = 0;
    while(!connection_closed)
    {
        FD_SET(fileno(stdin), &fds);
        FD_SET(sck_fd, &fds);
        mx = maximum(fileno(stdin) + 1, sck_fd + 1);

        select(mx, &fds, NULL, NULL, NULL);

        if(FD_ISSET(fileno(stdin), &fds))
        {
            if(DEBUG)
            { 
                printf("reading from stdin\n"); 
            }

            int num_read = getline(&user_command, &user_max_length, stdin);

            if(num_read == -1)
            {
                printf("error getline()\n");
                exit(0);
            }
            write(sck_fd, user_command, num_read);
            add_command_to_server(server, user_command);
            write_servers_to_file("servers.txt", servers, count);
        }

        if(FD_ISSET(sck_fd, &fds))
        {
            int num_read = read(sck_fd, inp_buf, BUFFER_SZ);

            if(DEBUG)
            { 
                printf("reading from socket(%d bytes read)\n", num_read); 
            }

            if(num_read == -1)
            {
                printf("error read()\n");
                exit(0);
            }
            else if(num_read == 0)
            {
                write_servers_to_file("servers.txt", servers, count);
                printf("Connection closed\n");
                connection_closed = 1;
            }
            else
            {
                handle_Server_Data(sck_fd, inp_buf, num_read);
            }
        }
    }
}

void connect_to_server(Server servers[], int count) 
{
    printf("Choose a server to connect (enter server number): ");
    int choice;
    scanf("%d", &choice);
    fflush(stdout); 
    fflush(stdin); 
    if (choice < 1 || choice > count) {
        printf("Invalid choice\n");
        return;
    }

    Server *selected_server = &servers[choice - 1];

    struct addrinfo hints;
    struct addrinfo *results;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", selected_server->port);
    if(getaddrinfo(selected_server->address, port_str, &hints, &results) != 0)
    {
        printf("error getaddrinfo()\n");
        return;
    }

    struct addrinfo *addr_okay;
    int succeeded = 0;
    int sck_fd = socket(AF_INET, SOCK_STREAM, 0);

    for(addr_okay = results; addr_okay != NULL; addr_okay = addr_okay->ai_next)
    {
        if(connect(sck_fd, addr_okay->ai_addr, addr_okay->ai_addrlen) >= 0)
        {
            succeeded = 1;
            break;
        }
    }
    freeaddrinfo(results);

    if(!succeeded)
    {
        printf("error connect()\n");
        return;
    }

    start_communication(sck_fd, selected_server, servers, count);
    close(sck_fd);

}

int main(int argc, char *argv[])
{ 
    if(argc == 2)
    {
        if (strcmp(argv[1], "L1") == 0) 
        {
            Server servers[MAX_SERVERS];

            int count = read_servers_from_file("servers.txt", servers);

            if (count > 0) 
            {
                print_servers(servers, count);
                connect_to_server(servers, count);
                write_servers_to_file("servers.txt", servers, count);
            } 
            else 
            {
                printf("No servers found in file\n");
            }
            write_servers_to_file("servers.txt", servers, count);
        } 
        else if (strcmp(argv[1], "L2") == 0) 
        {
            Server servers[MAX_SERVERS];

            int count = read_servers_from_file("servers.txt", servers);

            if (count > 0) 
            {
                print_servers(servers, count);
                int server_choice;
                printf("Choose a server to show commands (enter server number): ");
                scanf("%d", &server_choice);
                show_commands_for_server(servers, count, server_choice);
            } 
            else 
            {
                printf("No servers found in file\n");
            }
        }
    }
    else if(argc < 3)
    {
        printf("Basic usage: <file> <ip_address> <port_no>\n");
        return 0;
    }
    else
    {
        // Checking if server already exists in the list
        Server servers[MAX_SERVERS];
        int count = read_servers_from_file("servers.txt", servers);

        int exists = 0;
        int server_index = -1;
        for (int i = 0; i < count; i++) {
            if (strcmp(servers[i].address, argv[1]) == 0 && servers[i].port == atoi(argv[2])) {
                exists = 1;
                server_index = i;
                break;
            }
        }

        Server *selected_server;
        if (exists) {
            selected_server = &servers[server_index];
        } else {
            // Adding new server to the list if it does not exist
            if (count < MAX_SERVERS) {
                selected_server = &servers[count++];
                strncpy(selected_server->address, argv[1], MAX_ADDRESS_LENGTH - 1);
                selected_server->address[MAX_ADDRESS_LENGTH - 1] = '\0';
                selected_server->port = atoi(argv[2]);
                selected_server->commands_count = 0;
            } else {
                printf("Maximum number of servers reached\n");
                return 0;
            }
        }
        write_servers_to_file("servers.txt", servers, count);
        struct addrinfo hints;
        struct addrinfo *results;
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = 0;
        hints.ai_protocol = 0;

        char port_str[6];
        snprintf(port_str, sizeof(port_str), "%d", selected_server->port);
        if(getaddrinfo(selected_server->address, port_str, &hints, &results) != 0)
        {
            printf("error getaddrinfo()\n");
            return 0;
        }
        struct addrinfo *addr_okay;
        int succeeded = 0;
        int sck_fd = socket(AF_INET, SOCK_STREAM, 0);

        for(addr_okay = results; addr_okay != NULL; addr_okay = addr_okay->ai_next)
        {
            if(connect(sck_fd, addr_okay->ai_addr, addr_okay->ai_addrlen) >= 0)
            {
                succeeded = 1;
                break;
            }
        }
        freeaddrinfo(results);
        if(!succeeded)
        {
            printf("error connect()\n");
            return 0;
        }
        start_communication(sck_fd, selected_server, servers, count);
        close(sck_fd);

        write_servers_to_file("servers.txt", servers, count);

    }

    return 0;
}

