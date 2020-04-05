#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifndef LOG_H_
#define LOG_H_

#define MAX_LEN 100

FILE *createLog();
void writeLog(FILE *fd, char *string);
void closeLog(FILE *fd);

#endif
