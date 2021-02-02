#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>

#define DEFAULT_PORT 8081

typedef struct {
    int p2[2];
    int p3[3];
    int p4[4];
} waiting_list;


char* itoa(int num, char* str) {
    char digits[] = "0123456789";
    char temp[16];
    int i = 0, j = 0;
    do {
        temp[i++] = digits[num % 10];
        num /= 10;
    } while (num > 0);
    
    while (--i >= 0)
        str[j++] = temp[i];
    str[j] = '\0';

    return str;
}

int get_rand_port() {
    struct sockaddr_in addr;
    int test_sock;
    addr.sin_family = AF_INET; 
    addr.sin_port = htons(0); 
    test_sock = socket(AF_INET, SOCK_DGRAM, 0);
    while (addr.sin_port == 0) {
        bind(test_sock, (struct sockaddr *)&addr, sizeof(addr));
        int len = sizeof(addr);
        getsockname(test_sock, (struct sockaddr *)&addr, &len );
    }
    close(test_sock);
    return addr.sin_port;
}

int add_player(int sock, int group, waiting_list* list) {
    int i;
    char temp[16];
    int *group_list = group == 2 ? list->p2 : group == 3 ? list->p3 : list->p4;
    for (i = 0; i < group; i++){
        if (group_list[i] == 0) {
            group_list[i] = sock;
            break;
        }
    }

    itoa(group, temp);
    write(1, "New player added to waiting list for ", 37);
    write(1, temp, strlen(temp));
    write(1, " Players\n", 9);

    if (i == group - 1)
        return 1;

    return 0;
}

void send_port(int group, waiting_list* list) {
    int *group_list = group == 2 ? list->p2 : group == 3 ? list->p3 : list->p4;
    int port = get_rand_port();
    char buf[128] = {0};
    char temp[16] = {0};

    for (int i = 0; i < group; i++){
        itoa(port, buf);
        strcat(buf, "#");
        strcat(buf, itoa(i, temp));
        strcat(buf, "#");
        send(group_list[i], buf, strlen(buf), 0);
        close(group_list[i]);
        group_list[i] = 0;
    }
    itoa(port, temp);
    write(1, "Group is full, Port ", 20);
    write(1, temp, strlen(temp));
    write(1, " sent!\n", 7);
}

int main(int argc, char const *argv[])
{
    int server_fd, new_socket, valread, max_sd, port;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char temp[128] = {0};
    char port_str[10] = {0};
    fd_set master_set, working_set;
    waiting_list players_list = {0};
       
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    opt = fcntl(server_fd, F_GETFL);
    opt = (opt | O_NONBLOCK);
    fcntl(server_fd, F_SETFL, opt);

    port = argc > 1 ? atoi(argv[1]) : DEFAULT_PORT;
    itoa(port, port_str);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
       
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 4);

    FD_ZERO(&master_set);
    max_sd = server_fd;
    FD_SET(server_fd, &master_set);

    write(1, "Server is running on port ", 26);
    write(1, port_str, strlen(port_str));
    write(1, "\nWaiting for players...\n", 24);

    while (1) {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);

        for (int i = 0; i <= max_sd; i++) {
            if (FD_ISSET(i, &working_set)) {
                if (i == server_fd) {
                    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                    write(1, "New player connected\n", 21);
                }
                else {
                    valread = recv(i , buffer, 1024, 0);
                    int group_num = atoi(strtok(buffer, "#"));
                    int ready = add_player(i, group_num, &players_list);
                    if (ready)
                        send_port(group_num, &players_list);
                    FD_CLR(i, &master_set);
                    
                }
            }
        }

    }

    return 0; 
} 