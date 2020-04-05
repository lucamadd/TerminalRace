#include "player.h"

void insertTopPlayerList(playerList_p *L, player_p val) {
	if(*L == NULL){
		*L = (playerList_p) malloc(sizeof(playerList));
		(*L)->player = val;
		(*L)->next = NULL;
	}
	else{
		playerList_p top = (playerList_p) malloc(sizeof(playerList));
		top->player = val;
		top->next = *L;
		*L = top;
	}
}

void removeFromPlayerList(playerList_p *top, playerList_p *parent, int client_fd) {
	if((*top) == NULL){
		return;
	}
	else {
		if((*top)->player->cod == client_fd){
			playerList_p tmp = *top;
			if((*parent) == NULL)
				*top = (*top)->next;
			else
				(*parent)->next = (*top)->next;
			free(tmp);
		}
		else
			removeFromPlayerList(&(*top)->next, top, client_fd);
	}
}

player_p getPlayerFromPlayerList(playerList_p top, int cod) {
	if(top == NULL)
		return NULL;
	else{
		if(top->player->cod == cod)
			return top->player;
		else
			getPlayerFromPlayerList(top->next, cod);
	}
}

player_p getPlayerByCoordFromPlayerList(playerList_p top, int x, int y) {
	if(top == NULL)
		return NULL;
	else{
		if(top->player->x == x && top->player->y == y)
			return top->player;
		else
			getPlayerByCoordFromPlayerList(top->next, x, y);
	}
}

void printPlayerList(playerList_p top) {
	if(top == NULL)
		printf("END");
	else{
		printf("%d->", top->player->cod);
		printPlayerList(top->next);
	}
}

void freePlayerList(playerList_p *top) {
	if(*top != NULL){
		freePlayerList(&(*top)->next);
		free(*top);
		*top = NULL;
	}
}

void insertTailPlayerList2(playerList_p *L, playerList_p *prev, player_p val) {
	if (*L == NULL){
		playerList_p tmp = (playerList_p) malloc(sizeof(playerList));
		tmp->player = val;
		tmp->next = NULL;
		if (*prev != NULL)
			(*prev)->next = tmp;
		else
			*L = tmp;
	}
	else
		if ((*L)->player->cod != val->cod)
			insertTailPlayerList2(&(*L)->next, L, val);
}

void insertTailPlayerList(playerList_p *L, player_p val) {
	playerList_p prev = NULL;
	insertTailPlayerList2(L, &prev, val);
}
