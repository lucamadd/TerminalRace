#include <stdlib.h>
#include <stdio.h>

#ifndef PLAYER_H_
#define PLAYER_H_


typedef struct player {
	char *name;           //player name
	int cod;	      //player code
	int inGame;	      //1 if player is online
	int x;		      //x position
	int y;		      //y position
} player, *player_p;

typedef struct playerList {
	player_p player;
	struct playerList *next;
} playerList, *playerList_p;

void insertTopPlayerList(playerList_p *L, player_p val);
void insertTailPlayerList(playerList_p *L, player_p val);
void removeFromPlayerList(playerList_p *top, playerList_p *parent, int client_fd);
void printPlayerList(playerList_p top);
player_p getPlayerFromPlayerList(playerList_p top, int cod);
player_p getPlayerByCoordFromPlayerList(playerList_p top, int x, int y);
void freePlayerList(playerList_p *top);

#endif
