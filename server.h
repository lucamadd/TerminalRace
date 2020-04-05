#include "log.h"
#include "settings.h"
#include "InputLib.h"
#include "player.h"

#ifndef SERVER_H_
#define SERVER_H_


void createUser(char *buffer, int client_fd);
void loginUser(char *buffer, int client_fd);
void spawnUser(char *buffer, int client_fd);
int  checkToken(char *buffer);
void sendEffect(player_p plyr, char *effect);
void movePlayer(player_p plyr, int x, int y);
void deletePlayer(int client_fd);
void printMap();
void createMap();
void setupServer();

void *serverLog(void *args);
void* g_start_timer(void *args);

void broadcast_gameOver();
void broadcast_timer();
void broadcast_new_game();
void broadcast_wall_list();
void broadcast_player_position(player_p plyr);
void broadcast_new_player(player_p plyr);
void broadcast_player_left(int client_fd);
void broadcast_win_cell();

void send_player_list_to(player_p plyr);
void send_wall_list_to(player_p plyr);

void rot13_crypt();
void rot13_decrypt();

typedef struct gameMap {
	int type;
	int discovered;
} gameMap;

typedef struct ClientAccount {
    int account_number;
    sem_t sem;
    int writing;
} ClientAccount;

typedef struct ClientAccountList {
	ClientAccount *account;
	struct ClientAccountList *next;
} ClientAccountList, *ClientAccountList_p;

typedef struct broadcast_data {
	int inGame;
	int cod;
	char *buffer;
} broadcast_data;


#endif
