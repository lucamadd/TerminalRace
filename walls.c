#include "walls.h"

void insertTailWallList2(wallList_p *L, wallList_p *prev, wall_p val) {
	if (*L == NULL){
		wallList_p tmp = (wallList_p) malloc(sizeof(wallList));
		tmp->wall = val;
		tmp->next = NULL;
		if (*prev != NULL)
			(*prev)->next = tmp;
		else
			*L = tmp;
	}
	else
		if ((*L)->wall->x != val->x || (*L)->wall->y != val->y)
			insertTailWallList2(&(*L)->next, L, val);
}

void insertTailWallList(wallList_p *L, wall_p val) {
	wallList_p prev = NULL;
	insertTailWallList2(L, &prev, val);
}

int isPresentInWallList(wallList_p *top, int x, int y) {
	if (*top != NULL){
		if((*top)->wall->x == x && (*top)->wall->y == y)
			return 1;
		else
			isPresentInWallList(&(*top)->next, x, y);
	}
	else
		return 0;
}

int isWinnerInWallList(wallList_p *top, int x, int y) {
	if (*top != NULL){
		if((*top)->wall->x == x && (*top)->wall->y == y)
			return 1;
		else
			isWinnerInWallList(&(*top)->next, x, y);
	}
	else
		return 0;
}

void freeWallList(wallList_p *top) {
	if(*top != NULL){
		freeWallList(&(*top)->next);
		free(*top);
		*top = NULL;
	}
}

void printWallList(wallList_p top) {
	if(top == NULL)
		printf("END\n");
	else{
		printf("(%d;%d)->", top->wall->x, top->wall->y);
		printWallList(top->next);
	}
}
