#include <stdio.h>
#include <sys/time.h>
#include <sys/termios.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "settings.h"


//Clear function for UNIX Terminal
#define clear() printf("\e[H\e[J\e[3J")

void clearStdIn();

//Functions to safe get integer from @scanf()
int getInt(int* address);
int getPositiveInt(int *address);
int getNonNegativeInt(int *address);
int getRangeInt(int *address, int min, int max);

int getMenuChoice(int min, int max);

void* newMalloc(size_t size);
