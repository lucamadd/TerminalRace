#include "log.h"

//create log file
FILE *createLog() {
	char name[MAX_LEN];
	char date_time[100];
	time_t now = time(0);
	strftime(date_time, 100, "%Y-%m-%d %H:%M:%S.000", localtime(&now));
	sprintf(name, "log_%lld.txt", (long long)time(NULL));
	FILE *fd = fopen(name, "ab+");
	if(fd) {
		fprintf(fd, "---------- New log: %s ----------\n\n", date_time);
	}
	return fd;
}


//write activities
void writeLog(FILE *fd, char *string) {
	char date_time[100];
	time_t now = time(0);
	strftime(date_time, 100, "%Y-%m-%d %H:%M:%S.000", localtime(&now));
	fprintf(fd, "%s: %s\n", date_time, string);
}

//close file
void closeLog(FILE *fd) {
	fclose(fd);
}
