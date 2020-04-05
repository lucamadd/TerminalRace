#include <stdlib.h>
#include <stdio.h>



typedef struct wall {
	int x;
	int y;
} wall, *wall_p;

typedef struct wallList {
	wall_p wall;
	struct wallList *next;
} wallList, *wallList_p;

void insertTailWallList(wallList_p *L, wall_p val);
void freeWallList(wallList_p *top);
int isPresentInWallList(wallList_p *top, int x, int y);
int isWinnerInWallList(wallList_p *top, int x, int y);
void printWallList(wallList_p top);
