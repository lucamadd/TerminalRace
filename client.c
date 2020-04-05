#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/termios.h>
#include <sys/un.h>
#include <pthread.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "client.h"



int timeout, spawn = 0, waiting_move = 0;
player_p plyr;
char *info, *name, *token;

wallList_p wallsList;

playerList_p onlinePlayers;
int winnerX, winnerY;  //final positions

static struct termios g_old_kbd_mode;


static int kbhit(void) {
	struct timeval timeout;
	fd_set read_handles;
	int status;

	FD_ZERO(&read_handles);
	FD_SET(0, &read_handles);
	timeout.tv_sec = timeout.tv_usec = 0;
	status = select(0 + 1, &read_handles, NULL, NULL, &timeout);
	return status;
}

//Restore default input mode
static void old_attr(void){
	tcsetattr(0, TCSANOW, &g_old_kbd_mode);
}

void move(int client_fd, int x, int y) {
	char bufferOut[BUFFER_SIZE];
	sprintf(bufferOut, "MOVE&%d&%d", x, y);
	write(client_fd, bufferOut, strlen(bufferOut));
}

void printOnlineList() {
	playerList_p tmp = onlinePlayers;
	printf(BOLD " Online users:\n");
	printf(BOLDRED "\t- Me\n\n");
	while(tmp != NULL) {
		printf(BOLDBLUE "\t- (%d) %s\n\n", tmp->player->cod, tmp->player->name);
		tmp = tmp->next;
	}
}


void drawGameUI(int client_fd) {
	clear();
	printf(BOLD UNDERLINE "\n\t\t\t\t  GAME MAP  \n\n" DEFAULT);
	int i, j;
	printf("\t\t\b\b");
	for(i = 0; i < MAP_SIZE+1; i++){
			printf(FGGREEN BGGREEN "...");
	}
	printf(FGGREEN BGGREEN "." DEFAULT "\n");
	for(i = 0; i < MAP_SIZE; i++){
		printf("\t\t");
		for(j = 0; j < MAP_SIZE; j++){
			if (j==0)
				printf(FGGREEN BGGREEN "\b\b.." DEFAULT);
			player_p p = getPlayerByCoordFromPlayerList(onlinePlayers, i, j);
			if(i == plyr->x && j == plyr->y) {
				if (winnerX==i && winnerY==j)
					printf(FGLIGHTMAGENTA BGLIGHTGREEN BLINK "[" DEFAULTBLINK BOLDRED "@" DEFAULT FGLIGHTMAGENTA BGLIGHTGREEN BLINK "]" DEFAULTBLINK DEFAULT);
				else
					printf(FGGREEN BGLIGHTGREEN ". " BOLDRED "@" DEFAULT);
			}
			else if (isPresentInWallList(&wallsList, i, j))
				printf(FGGREEN BGGREEN "...");
			else if (p != NULL)
				printf(FGGREEN BGLIGHTGREEN "." BOLDBLUE "%2d" DEFAULT , p->cod);
			else if (winnerX==i && winnerY==j)
				printf(FGGREEN BGLIGHTGREEN "." FGLIGHTMAGENTA BGLIGHTGREEN BLINK "[]" DEFAULTBLINK DEFAULT);
			else
				printf(FGGREEN BGLIGHTGREEN ".  " DEFAULT);
			if (j==MAP_SIZE-1)
					printf(FGGREEN BGGREEN "\t\b\b..\n" DEFAULT);
		}
	}
	printf("\t\t\b\b");
	for(i = 0; i < MAP_SIZE+1; i++)
		printf(FGGREEN BGGREEN "...");
	printf(FGGREEN BGGREEN "." DEFAULT "\n");

	printf(BOLD  "\n-------------------------------------------------------------------------------\n");
	printf(" CURRENT POSITION  X: %d Y: %d \t\t %s", plyr->x, plyr->y, info);
	printf(BOLD "\n-------------------------------------------------------------------------------\n");
	printf(BOLDGREEN " MOVEMENTS:" DEFAULT BOLD " ↑ ← ↓ → / w a s d \t\t" BOLDGREEN " LOGOUT:" DEFAULT BOLD " q \n");
	printf(BOLD "\n---------------------------\n" DEFAULT);
	printOnlineList();
	printf(DEFAULT BOLD "---------------------------\n" DEFAULT);
}


void drawGameOverUI(int x,int y,int cod,int client_fd) {
	clear();
	if(info != NULL)
		free(info);
	info = NULL;
  if (plyr->x ==winnerX && plyr->y==winnerY){
		printf(BOLDGREEN "\n\n\t\t**************************************\n");
		printf(BOLDGREEN "\t\t*              YOU WIN!              *\n");
		printf(BOLDGREEN "\t\t**************************************\n\n");
  }
	else{
		printf(BOLDRED "\n\n\t\t**************************************\n");
		printf(BOLDRED "\t\t*              YOU LOSE!             *\n");
		printf(BOLDRED "\t\t**************************************\n\n");
	}
		printf( DEFAULT BOLD "\n\nNew game in %2d\n", timeout);
}

void drawInfo() {
	if(info != NULL)
		printf("%s\n", info);
}

void drawSpawnUI(int client_fd) {
	clear();
	printf(BOLD FGLIGHTBLUE "Terminal Race\n\n\n" DEFAULT);

	if(plyr == NULL)
		plyr = newMalloc(sizeof(player));

	freeWallList(&wallsList);
	freePlayerList(&onlinePlayers);

	drawInfo();

	char bufferOut[BUFFER_SIZE];
	sprintf(bufferOut, "SPAWN&%s&%s&", name, token);
	plyr->inGame=1;
	plyr->y=0;
	plyr->x=rand()%MAP_SIZE;

	sprintf(bufferOut + strlen(bufferOut), "%d&%d&%d", plyr->inGame, plyr->x, plyr->y);
	write(client_fd, bufferOut, strlen(bufferOut));
}




void drawLoginUI(int client_fd) {
	clear();
	printf(BOLD FGLIGHTBLUE "Terminal Race\n\n\n" DEFAULT);

	char s[BUFFER_SIZE];
	char bufferOut[BUFFER_SIZE+10];

	drawInfo();

	printf("Insert your username ('q' to quit): ");
	scanf("%s", s);
	if(s[0] == 'q'){
		drawMainUI(client_fd);
		return;
	}
	sprintf(bufferOut, "LOGIN&%s&", s);

	printf("Insert your password: \e[8m");
	scanf("%s", s);
	sprintf(bufferOut + strlen(bufferOut), "%s", s);
	printf("\e[28m");
	write(client_fd, bufferOut, strlen(bufferOut));
}


void drawRegisterUI(int client_fd) {
	clear();
	printf(BOLD FGLIGHTBLUE "Terminal Race\n\n\n" DEFAULT);

	char s[BUFFER_SIZE];
	char bufferOut[BUFFER_SIZE+10];

	drawInfo();

	printf("Insert your new username ('q' to quit): ");
	scanf("%s", s);
	if(s[0] == 'q'){
		drawMainUI(client_fd);
		return;
	}
	sprintf(bufferOut, "REGISTER&%s&", s);

	printf("Insert your new password: ");
	scanf("%s", s);
	sprintf(bufferOut + strlen(bufferOut), "%s", s);

	write(client_fd, bufferOut, strlen(bufferOut));
}

void drawMainUI(int client_fd) {
	clear();

	printf(BOLD FGLIGHTBLUE "Terminal Race\n" DEFAULT);
	printf(BOLDGREEN "\n\t1." DEFAULT " Login\n"); //1
	printf(BOLDGREEN "\t2." DEFAULT " Register\n"); //2
	printf(BOLDGREEN "\t3." DEFAULT " Quit\n"); //3

	switch (getMenuChoice(1, 3)) {
	case 1:
		drawLoginUI(client_fd);
		break;
	case 2:
		drawRegisterUI(client_fd);
		break;
	default:
		exit(0);
	}
}


void saveToken(char *buffer, int client_fd) {
	int i = 0;

	name = newMalloc(sizeof(char)*BUFFER_SIZE);
	token = newMalloc(sizeof(char)*BUFFER_SIZE);

	char *msg = strtok(buffer, "&");
	msg = strtok(NULL, ":");
	while(msg != NULL){
		if(i == 0)
			sprintf(name, "%s", msg);
		else if (i == 1)
			sprintf(token, "%s", msg);
		i++;
		msg = strtok(NULL, ":");
	}
	write(client_fd, "JOIN&", strlen("JOIN&"));
	free(info);
	info = NULL;
}

void handleError(char *buffer, int client_fd) {
	int i = 0;

	if(info == NULL)
		info = newMalloc(sizeof(char)*BUFFER_SIZE);
	char *msg = strtok(buffer, "&");
	while(msg != NULL){
		if(i == 1)
			sprintf(info, BOLDRED "%s\n\n" DEFAULT, msg);
		i++;
		msg = strtok(NULL, "&");
	}
}

void handleInfo(char *buffer, int client_fd) {
	int i = 0;

	if(info == NULL)
		info = newMalloc(sizeof(char)*BUFFER_SIZE);
	char *msg = strtok(buffer, "&");
	while(msg != NULL){
		if(i == 1)
			sprintf(info, BOLDCYAN "%s\n\n" DEFAULT, msg);
		i++;
		msg = strtok(NULL, "&");
	}
}

void handleMap(char *buffer, int client_fd) {
	int i = 0;
	if(info == NULL)
		info = newMalloc(sizeof(char)*BUFFER_SIZE);

	char *msg = strtok(buffer, "&");
	while(msg != NULL){
		if (i == 1)
			plyr->x = atoi(msg);
		else if (i == 2)
			plyr->y = atoi(msg);
		else if (i == 3)
			sprintf(info,BOLDCYAN "%s\n\n" DEFAULT, msg);
		i++;
		msg = strtok(NULL, "&");
	}
	waiting_move = 0;
}

void handleTimeout(char *buffer, int client_fd) {
	int i = 0;
	char *msg = strtok(buffer, "&");
	while(msg != NULL) {
		if(i == 1)
			timeout = atoi(msg);
		i++;
		msg = strtok(NULL, "&");
	}
}

void handleWalls(char *buffer, int client_fd) {
	int i = 0;
	printf("%s", buffer);
	char *msg = strtok(buffer, "&");
	wall_p wall;
	while(msg != NULL) {
		if(i > 0) {
			if (i % 2 != 0) {
				wall = (wall_p) newMalloc(sizeof(wall));
				wall->x = atoi(msg);
			}
			else {
				wall->y = atoi(msg);
				insertTailWallList(&wallsList, wall);
			}
		}
		i++;
		msg = strtok(NULL, "&");
	}
}

void handleWin(char *buffer, int client_fd) {
	int i = 0;
	printf("%s", buffer);
	//while (1){
	//	printf("%s\n",buffer );
	//}
	char *msg = strtok(buffer, "&");
	while(msg != NULL) {
		if (i==1)
			winnerX=atoi(msg);
		else if (i==2)
			winnerY=atoi(msg);
		i++;
		msg = strtok(NULL, "&");
	}
}

void handlePlayers(char *buffer, int client_fd) {
	char *msg = strtok(buffer, "&");
    int i = 0, cod, x, y;
    char *name = (char*) newMalloc(BUFFER_SIZE * sizeof(char));
    while(msg != NULL) {
      	if (i == 2)
      		cod = atoi(msg);
      	else if (i == 3)
      	 	x = atoi(msg);
   			else if (i == 4)
      	 	y = atoi(msg);
   			else if (i == 1)
      	 	sprintf(name, "%s", msg);
      	i++;
				msg = strtok(NULL, "&");
   	}
   	player_p p = getPlayerFromPlayerList(onlinePlayers, cod);
   	if(p == NULL){
   		p = newMalloc(sizeof(player));
   		insertTopPlayerList(&onlinePlayers, p);
   	}

   	p->name = name;
   	p->cod = cod;
   	p->x = x;
   	p->y = y;
		p->inGame=1;
}

void handlePlayerLeft(char *buffer, int client_fd) {
	char *msg = strtok(buffer, "&");
	msg = strtok(NULL, "&");
	int cod = atoi(msg);
	printf("cod to remove: %d\n", cod);
	playerList_p p = NULL;
  removeFromPlayerList(&onlinePlayers, &p, cod);
}


void handleGameOver(char *buffer, int client_fd) {
	char *msg = strtok(buffer, "&");
	int i=0,x,y,cod;
	while (msg!=NULL){
		if (i==1)
			x=atoi(msg);
	  else if (i==2)
		  y=atoi(msg);
		else if (i==3)
			cod=atoi(msg);
	  i++;
    msg = strtok(NULL, "&");
	}
	spawn = 0;
	drawGameOverUI(x,y,cod,client_fd);
}

long timediff(clock_t t1, clock_t t2) {
    long elapsed;
    elapsed = ((double)t2 - t1) / CLOCKS_PER_SEC;
    return elapsed;
}

void *handleInput(void *args) {

	//Save default input mode
	struct termios new_kbd_mode;
	tcgetattr(0, &g_old_kbd_mode);
	memcpy(&new_kbd_mode, &g_old_kbd_mode, sizeof(struct termios));

	int client_fd = *((int*) args);
	char c;
	while(1) {
	 	if(spawn){
			new_kbd_mode.c_lflag &= ~(ICANON | ECHO);
			new_kbd_mode.c_cc[VTIME] = 0;
			new_kbd_mode.c_cc[VMIN] = 1;
			tcsetattr(0, TCSANOW, &new_kbd_mode);
			atexit(old_attr);

	 		while(!kbhit() && spawn){
	 			c = getchar();

	 			if(waiting_move)
	 				continue;

	 			switch (c) {
                    case '\033':
					getchar();
					switch (getchar()){
							case 'A':
							waiting_move = 1;
							move(client_fd, plyr->x - 1, plyr->y);
									break;
							case 'D':
							waiting_move = 1;
							move(client_fd, plyr->x, plyr->y - 1);
									break;
							case 'B':
							waiting_move = 1;
							move(client_fd, plyr->x + 1, plyr->y);
									break;
							case 'C':
							waiting_move = 1;
							move(client_fd, plyr->x, plyr->y + 1);
									break;
					}
						break;
					case 'a':
						waiting_move = 1;
						move(client_fd, plyr->x, plyr->y - 1);
						break;
					case 'w':
						waiting_move = 1;
						move(client_fd, plyr->x - 1, plyr->y);
						break;
					case 's':
						waiting_move = 1;
						move(client_fd, plyr->x + 1, plyr->y);
						break;
					case 'd':
						waiting_move = 1;
						move(client_fd, plyr->x, plyr->y + 1);
						break;
					case 'q':
						spawn = 0;
						old_attr();
						write(client_fd, "LOGOUT&", strlen("LOGOUT&"));
						break;
				}
	 		}
		}
	}
}

void* callbackServer(void *args) {
  int client_fd = *((int*) args);

	int ret = write(client_fd, "WELCOME&", strlen("WELCOME&"));
	char buffer[BUFFER_SIZE];

	char check[2];
	check[0] = 'Y';
	check[1] = '\0';

	while (ret) {
			ret = read(client_fd, buffer, BUFFER_SIZE);
			write(client_fd, check, strlen(check));
			buffer[ret] = '\0';
			if(ret != 0){
				if(strstr(buffer, "ACCESS&") != NULL)
					saveToken(buffer, client_fd);
				else if (strstr(buffer, "JOINED&") != NULL) {
					printf("Press ENTER to continue\n");
					old_attr();
					clearStdIn();
					drawSpawnUI(client_fd);
				}
				else if (strstr(buffer, "WAITJOIN&") != NULL)
					drawGameOverUI(-1,-1,-1,client_fd);
				else if (strstr(buffer, "ERROR&Invalid username or password") != NULL) {
					handleError(buffer, client_fd);
					drawLoginUI(client_fd);
				}
				else if (strstr(buffer, "ERROR&User already in game") != NULL) {
					handleError(buffer, client_fd);
					drawLoginUI(client_fd);
				}
				else if (strstr(buffer, "INFO&Register success!") != NULL) {
					handleInfo(buffer, client_fd);
					drawLoginUI(client_fd);
				}
				else if (strstr(buffer, "ERROR&Username already taken") != NULL) {
					handleError(buffer, client_fd);
					drawRegisterUI(client_fd);
				}
				else if (strstr(buffer, "ERROR&Invalid access token") != NULL) {
					handleError(buffer, client_fd);
					drawLoginUI(client_fd);
				}
				else if (strstr(buffer, "User spawned") != NULL) {
					spawn = 1;
					handleMap(buffer, client_fd);
					drawGameUI(client_fd);
				}
				else if (strstr(buffer, "ERROR&Invalid position") != NULL) {
					drawSpawnUI(client_fd);
				}
				else if (strstr(buffer, "INFO&User logged out") != NULL)
					break;
				else if (strstr(buffer, "ERROR&") != NULL)
					handleError(buffer, client_fd);
				else if (strstr(buffer, "INFO&") != NULL)
					handleInfo(buffer, client_fd);
				else if (strstr(buffer, "MOVED&") != NULL) {
					handleMap(buffer, client_fd);
					drawGameUI(client_fd);
				}
				else if (strstr(buffer, "FIGHT&") != NULL) {
					handleMap(buffer, client_fd);
					drawGameUI(client_fd);
				}
				else if (strstr(buffer, "TIMEOUT&") != NULL) {
					handleTimeout(buffer, client_fd);
					if(spawn)
						drawGameUI(client_fd);
					else {
						drawGameOverUI(0,0,0,client_fd);
						spawn=0;
					}
				}
				else if (strstr(buffer, "WALLS&") != NULL && spawn) {
					handleWalls(buffer, client_fd);
					drawGameUI(client_fd);
				}
				else if (strstr(buffer, "WIN&") != NULL) {
					handleWin(buffer, client_fd);
					drawGameUI(client_fd);
				}
				else if (strstr(buffer, "USERS&") != NULL && spawn) {
					handlePlayers(buffer, client_fd);
					drawGameUI(client_fd);
				}
				else if (strstr(buffer, "LEFT&") != NULL && spawn) {
					handlePlayerLeft(buffer, client_fd);
					drawGameUI(client_fd);
				}
				else if (strstr(buffer, "WELCOME&") != NULL)
					drawMainUI(client_fd);
				else if (strstr(buffer, "GAMEOVER&") != NULL) {
					spawn = 0;
					handleGameOver(buffer, client_fd);
				}
			}
			bzero(buffer, BUFFER_SIZE);
	}
    pthread_exit(NULL);
}

static void handle_interrupt(int signum) {
	if(signum == SIGINT || signum == SIGHUP || signum == SIGQUIT || signum == SIGTERM || signum == SIGKILL || signum == SIGPIPE){
		signal(signum, SIG_IGN);
		perror("\n\nDisconnected from server.\n");
		old_attr();
		if(wallsList != NULL)
			freeWallList(&wallsList);
		if(onlinePlayers != NULL)
			freePlayerList(&onlinePlayers);
		if(info != NULL)
			free(info);
		if(name != NULL)
			free(name);
		if(token != NULL)
			free(token);
		exit(0);
	}
}

int main(int argc, char **argv) {
	char ip[BUFFER_SIZE];
	int port;
	if(argc < 3) {
		printf("\nInsert IP address: ");
		scanf("%s",ip);
		printf("\nInsert port number: ");
		getPositiveInt(&port);
	}

	//Handle interrupt signals
	signal(SIGINT, handle_interrupt);
	signal(SIGHUP, handle_interrupt);
	signal(SIGQUIT, handle_interrupt);
	signal(SIGTERM, handle_interrupt);
	signal(SIGKILL, handle_interrupt);

	//Handle server disconnection
	signal(SIGPIPE, handle_interrupt);

    int client_fd;
    struct sockaddr_in address;

    bzero((char*)&address, sizeof(address));

    address.sin_family = AF_INET;
		if (argc < 3){
			address.sin_addr.s_addr = inet_addr(ip);
			address.sin_port = htons(port);
		}
		else{
    	address.sin_addr.s_addr = inet_addr(argv[1]);
    	address.sin_port = htons(atoi(argv[2]));
		}
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
    	perror("\nSocket creation error\n");
    	exit(-1);
    }

    if(connect(client_fd, (struct sockaddr *) &address, sizeof(address)) < 0){
        perror("Client: connect() failed\n");
        return -1;
    }

    pthread_t input_id;

	if (pthread_create(&input_id, NULL, handleInput, (void *) &client_fd) == 0) {
		pthread_detach(input_id);
	}


    wallsList = NULL;
    onlinePlayers = NULL;

    pthread_t cb_id;

	if (pthread_create(&cb_id, NULL, callbackServer, (void *) &client_fd) == 0 ) {
		pthread_join(cb_id, NULL);
	}

	printf("\n\nDisconnected from server.\n");

    return 0;
}
