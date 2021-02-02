#include <unistd.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>


#define SCORED 2
#define NOT_SCORED 1
#define TIMEOUT 9

typedef struct {
    int** v_lines;
    int** h_lines;
    int** squares;
} Game;

void alarm_handler(int signum) {
    write(1, "20s passed without input!\n", 26);
}

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

void flush() {
    int c;
    char t[2];
    ioctl(0, FIONREAD, &c);
    while(c-- > 0)
        read(0,t,1);
}

void initialize_gb(Game* g, int player_count) {
    int n = player_count + 1;
    g->h_lines = malloc(n * sizeof(int*));
    g->v_lines = malloc((n - 1) * sizeof(int*));
    g->squares = malloc(n * sizeof(int*));

    for (int i = 0; i < n; i++) {
        g->h_lines[i] = malloc((n - 1) * sizeof(int));
        g->squares[i] = malloc(n * sizeof(int));
        if (i != n - 1)
            g->v_lines[i] = malloc(n * sizeof(int));

        for (int j = 0; j < n; j++) {
            g->squares[i][j] = -1;

            if (j != n - 1)
                g->h_lines[i][j] = 0;

            if (i != n - 1)
                g->v_lines[i][j] = 0;
        }
    }
}

void draw_map(Game* g, int plyaer_count) {
    char temp[10] = {0};
    write(1, "\n", 1);
	for (int i = 0; i < plyaer_count + 1; i++) {
		for (int j = 0; j < plyaer_count + 1; j++) {
			write(1, "*", 1);
			if (j == plyaer_count)
				break;
			if (g->h_lines[i][j])
				write(1, "--", 2);
			else
				write(1, "  ", 2);
		}
		write(1, "\n", 1);
		if (i == plyaer_count) 
			break;
		for (int j = 0; j < plyaer_count + 1; j++) {
			if (g->v_lines[i][j])
				write(1, "|", 1);
			else
				write(1, " ", 1);
			if (g->squares[i][j] != -1) {
                itoa(g->squares[i][j], temp);
				write(1, temp, strlen(temp)); 
                write(1, " ", 1);
            }
			else
				write(1, "  ", 2);
		}
		write(1, "\n", 1);
	}
}

int fill_board(Game *g, int id, int v, int i, int j, int player_count) {
    int has_scored = 0;
    if (v) 
        g->v_lines[i][j] = 1;
    else
        g->h_lines[i][j] = 1;

    for (int i = 0; i < player_count; i++) {
		for (int j = 0; j < player_count; j++) {
			if (g->h_lines[i][j] && g->h_lines[i + 1][j] && g->v_lines[i][j] && g->v_lines[i][j + 1] && g->squares[i][j] == -1) {
				g->squares[i][j] = id; 
                has_scored = 1;
			}
		}
	}
    
    return has_scored;
}

// returns next turn
int proccess_input(Game *g, char* input, int player_count) {
    int id, v, i, j, has_scored;
    char* tok = strtok(input, "#");
    id = atoi(tok);
    tok = strtok(NULL, "#");
    v = atoi(tok);
    if (v == TIMEOUT) {
        return TIMEOUT;
    }
    tok = strtok(NULL, "#");
    i = atoi(tok);
    tok = strtok(NULL, "#");
    j = atoi(tok);


    has_scored = fill_board(g, id, v, i, j, player_count);
    
    if (has_scored) {
        return SCORED;
    }

    return NOT_SCORED;
}


int is_game_finished(Game* g, int* winner, int player_count) {
    int count = 0, max = 0;
    int* scores = malloc(player_count * sizeof(int));

    for (int i = 0; i < player_count; i++)
        scores[i] = 0;

	for (int i = 0; i < player_count; i++) {
		for (int j = 0; j < player_count; j++) {
			if (g->squares[i][j] != -1) {
				count++;
                scores[g->squares[i][j]]++;
			}
		}
	}

    if (count == player_count * player_count) {
        for (int i = 0; i < player_count; i++)
            if(scores[i] > max)
                *winner = i;
        
        return 1;
    }

    return 0;
}

int main(int argc, char const *argv[]) {
    int sock, broadcast = 1, opt = 1, player_count, port, id, turn=0, prev_turn=0;
    char temp[100] = {0};
    char buffer[1024] = {0};
    struct sockaddr_in bc_address, server_address;
    char turn_info[128];
    Game game;

    siginterrupt(SIGALRM, 1);
    signal(SIGALRM, alarm_handler); 

    write(1, "How many players do want to play with? 2,3,4\n", 45);
    do {
        read(0, temp, 100);
        player_count = atoi(strtok(temp, "\n"));
        if (player_count < 2 || player_count > 4) {
            write(1, "Please enter a number between 2 and 4\n", 38);
            continue;
        }
        break;
    } while (1);


    // TCP: Get port from server
    sock = socket(AF_INET, SOCK_STREAM, 0);
   
    server_address.sin_family = AF_INET; 
    server_address.sin_port = htons(atoi(argv[1])); 
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr *)&server_address, sizeof(server_address));
    itoa(player_count, temp);
    strcat(temp, "#");
    send(sock , temp , strlen(temp) , 0 ); 
    write(1, "Connected to Server\n", 20);
    write(1, "Waiting for other players...\n", 29);
    recv(sock, buffer, 1024, 0);
    char* port_str = strtok(buffer, "#");
    char* id_str = strtok(NULL, "#");
    port = atoi(port_str);
    id = atoi(id_str);
    write(1, "Port ", 5);
    write(1, port_str, strlen(port_str));
    write(1, " Received, your ID is ", 22);
    write(1, id_str, strlen(id_str));
    write(1, "!\n", 2);
    close(sock);

    // UDP Run Game
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET; 
    bc_address.sin_port = htons(port); 
    bc_address.sin_addr.s_addr = inet_addr("255.255.255.255");

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));

    int v, i, j, winner=-1;
    initialize_gb(&game, player_count);
    turn = 0;
    write(1, "v indicates if the line is vertical or horizental\ni is row, j is column\n", 72);
    draw_map(&game, player_count);

    while(!is_game_finished(&game, &winner, player_count)) {
        if (turn == id) {
            write(1, "It's your turn! Enter v i j\n", 28);

            strcpy(turn_info, itoa(id, buffer));
            strcat(turn_info, "#");
            flush();
            strcpy(buffer, "9 9 9 9");
            alarm(20);
            read(0, buffer, 100);
            alarm(0);
            char *a;
            a = strtok(buffer, " \n");
            strcat(turn_info, a);
            strcat(turn_info, "#");
            a = strtok(NULL, " \n");
            strcat(turn_info, a);
            strcat(turn_info, "#");
            a = strtok(NULL, " \n");
            strcat(turn_info, a);
            strcat(turn_info, "#");
            sendto(sock, turn_info, strlen(turn_info), 0,(struct sockaddr *)&bc_address, sizeof(bc_address));
            turn = -1;
        }

        else {
            int score_state;
            if (turn != -1) {
                write(1, "It's player ", 12);
                itoa(turn, temp);
                write(1, temp, strlen(temp));
                write(1, "'s turn\n", 8);
            }

            recv(sock , buffer, 1024, 0); 

            score_state = proccess_input(&game, buffer, player_count);
            draw_map(&game, player_count);

            switch (score_state) {
            case SCORED:
                if (turn == -1)
                    turn = id;
                write(1, "Scored! Again ", 14);
                break;
            case TIMEOUT:
                write(1, "Player timed out!\n", 18);
            case NOT_SCORED:
                if (turn == -1)
                    turn = id;
                turn = (++turn) % player_count;
                break;            
            default:
                break;
            }
        }
    }

    write(1, "\nGame Finished!\nWinner is Player ", 39);
    itoa(winner, temp);
    write(1, temp, strlen(temp));
    write(1, "!\n", 2);
    close(sock);

    return 0;
}