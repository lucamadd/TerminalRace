#include "settings.h"
#include "InputLib.h"
#include "walls.h"
#include "player.h"


static int kbhit(void);
static void old_attr(void);


void move(int client_fd, int x, int y);
void printOnlineList();

void drawGameUI(int client_fd);
void drawGameOverUI(int x,int y,int cod,int client_fd);
void drawInfo();
void drawSpawnUI(int client_fd);
void drawLoginUI(int client_fd);
void drawRegisterUI(int client_fd);
void drawMainUI(int client_fd);

void saveToken(char *buffer, int client_fd);

void handleError(char *buffer, int client_fd);
void handleInfo(char *buffer, int client_fd);
void handleMap(char *buffer, int client_fd);
void handleTimeout(char *buffer, int client_fd);
void handleWalls(char *buffer, int client_fd);
void handleWin(char *buffer, int client_fd);
void handlePlayers(char *buffer, int client_fd);
void handlePlayerLeft(char *buffer, int client_fd);
void handleGameOver(char *buffer, int client_fd);


long timediff(clock_t t1, clock_t t2);
void *handleInput(void *args);
void* callbackServer(void *args);

static void handle_interrupt(int signum);
