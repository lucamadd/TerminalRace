#include "InputLib.h"

void clearStdIn() {
	write(STDOUT_FILENO, "\n", strlen("\n"));
	char a;
	do {
		scanf("%c", &a);
	} while (a != '\n');
}

//Get ONLY integer and nothing ELSE!
int getInt(int *address) {
	char a;
	//Check if @scanf() has matched 2 elements (the integer and \n), if not something unexpected happens
	while (scanf("%d%c", address, &a) != 2 || a != '\n') {
		printf(BOLDRED "ERRORE: " DEFAULT "Insert integers only: ");
		//Clear buffer first before ask a new number again
		do {
			scanf("%c", &a);
		} while (a != '\n');
	}
	return 1;
}

//Positive integers: ]0; +∞[
int getPositiveInt(int *address) {
	while (getInt(address) && *address <= 0) {
		printf(BOLDRED "ERRORE: " DEFAULT "Insert positive integers only: ");
	}
	return 1;
}

//Non negative integers: [0; +∞[
int getNonNegativeInt(int *address) {
	while (getInt(address) && *address < 0) {
		printf(BOLDRED "ERRORE: " DEFAULT "Insert non-negative integers only: ");
	}
	return 1;
}

//Range integers: [@min; @max]
int getRangeInt(int *address, int min, int max) {
	while (getInt(address) && (*address < min || *address > max)) {
		printf(BOLDRED "ERRORE: " DEFAULT "Insert numbers in range [%d,%d]: ", min, max);
	}
	return 1;
}

int getMenuChoice(int min, int max) {
	int c;
	printf("\nSelect an action [%d-%d]: ",min,max);
	getRangeInt(&c, min, max);
	return c;
}

void* newMalloc(size_t size) {
	void* p = malloc(size);
	if (p == NULL) {
		printf(BOLDRED "ERROR: " DEFAULT "Non enough memory!\n");
		exit(-1);
	}
	return p;
}
