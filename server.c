#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>

#include "server.h"

FILE *logFile;
pthread_t timer_tid;
playerList_p playersList;
gameMap **map;
int seconds=RESPAWN_TIMEOUT;
int DEBUG_MODE=0;

ClientAccountList_p g_account_list;

pthread_mutex_t g_list_mutex;
pthread_mutex_t g_map_mutex;


void freeAccountList(ClientAccountList_p *top) {
	if(*top != NULL){
		freeAccountList(&(*top)->next);
		free(*top);
		*top = NULL;
	}
}


ClientAccount* find_or_create_account(int account_number) {
    ClientAccount* account = NULL;
    pthread_mutex_lock(&g_list_mutex);
    ClientAccountList_p tmp = g_account_list;
    while (tmp != NULL) {
    	if (account_number == tmp->account->account_number) {
             account = tmp->account;
             break;
        }
        tmp = tmp->next;
    }
    if (account == NULL) {
       	// not found in the table, so create a new one
       	account = (ClientAccount*) newMalloc(sizeof(ClientAccount));
       	account->account_number = account_number;

    	sem_init(&account->sem, 0, 0);
    	account->writing = 0;

    	ClientAccountList_p top = (ClientAccountList_p) newMalloc(sizeof(ClientAccountList));
    	top->account = account;
    	top->next = g_account_list;
    	g_account_list = top;
    }

    // release the lock
    pthread_mutex_unlock(&g_list_mutex);
    return account;
}

void sendToClient(int client_fd, char *buffer){
	ClientAccount* account = find_or_create_account(client_fd);
  while(account->writing)
		sem_wait(&account->sem);
  account->writing = 1;
	write(client_fd, buffer, strlen(buffer));
	usleep(5 * 1000);
	account->writing = 0;
	sem_post(&account->sem);
}



void createUser(char *buffer, int client_fd) {
	int i = 0, duplicate = 0;
  char *msg, *tmpStr;
	msg = strtok(buffer, "&");
  char s[BUFFER_SIZE];
  char *line = (char*) newMalloc(BUFFER_SIZE * sizeof(char));
	rot13_decrypt();
  FILE* fd = fopen("users.txt", "ab+");
	while(msg != NULL && !duplicate){
		if(i == 1){
			sprintf(s, "%s", msg);
			while(fscanf(fd,"%s",line) != EOF) {
				tmpStr = strsep(&line, ":");
				if(tmpStr != NULL && strcmp(tmpStr, s) == 0){
					duplicate = 1;
					sendToClient(client_fd, "ERROR&Username already taken");
					char name[BUFFER_SIZE+80];
    				sprintf(name, "ERROR! %d tried to create an user with an already taken username: %s", client_fd, s);
						if (DEBUG_MODE==1){
							pthread_t logFile;
							if (pthread_create(&logFile, NULL,serverLog,name)==0)
								pthread_join(logFile, NULL);
					  	else
				       		perror("Thread creation error!\n");
				   	}
    				writeLog(logFile, name);
					  break;
				}
			}
		}
		else if(i == 2)
			sprintf(s + strlen(s),":%s\n", msg);
		i++;
		msg = strtok(NULL, "&");
	}
	if(!duplicate){
		fprintf(fd, "%s", s);
		sendToClient(client_fd, "INFO&Register success!");
		char name[BUFFER_SIZE+40];
    sprintf(name, "%d created new user: %s", client_fd, s);
		if (DEBUG_MODE==1){
			pthread_t logFile;
			if (pthread_create(&logFile, NULL, serverLog, name) == 0)
				pthread_join(logFile, NULL);
			else
				perror("Thread creation error!\n");
		}
  	writeLog(logFile, name);
	}
	fclose(fd);
	rot13_crypt();
}


void loginUser(char *buffer, int client_fd) {
	int i = 0, found = 0;
  char *msg;
	msg = strtok(buffer, "&");
	char s[BUFFER_SIZE+10];
  char *username = (char*) newMalloc(BUFFER_SIZE * sizeof(char));
  char *line = (char*) newMalloc(BUFFER_SIZE * sizeof(char));
  char token_string[BUFFER_SIZE];
	rot13_decrypt();
  FILE* fd_users_db = fopen("users.txt", "ab+");
  FILE* fd_tokens_db = fopen("tokens.txt", "ab+");
  while(msg != NULL){
    if(i == 1){
		  sprintf(s, "%s", msg);
			sprintf(username, "%s:", msg);
			sprintf(token_string, "%s:%d%d", msg, rand(), rand());
    }
		else if(i == 2)
			sprintf(s + strlen(s),":%s", msg);
    i++;
		msg = strtok(NULL, "&");
  }
	while(fscanf(fd_users_db,"%s",line) != EOF) {
		if(strcmp(line, s) == 0){
			found = 1;
			break;
		}
	}

	if(found){
		int exist = 0;
		while(fscanf(fd_tokens_db,"%s",line) != EOF) {
			if(strstr(line, username) != NULL){
				exist = 1;
				break;
			}
		}
		if (!exist) {
			sprintf(s, "ACCESS&%s", token_string);
			sendToClient(client_fd, s);
			char name[BUFFER_SIZE+50];
    	sprintf(name, "User %s - %d has logged in", token_string, client_fd);
			if (DEBUG_MODE==1){
				pthread_t logFile;
				if (pthread_create(&logFile, NULL, serverLog, name) == 0)
					pthread_join(logFile, NULL);
				else
					perror("Thread creation error!\n");
			}
    	writeLog(logFile, name);
			sprintf(token_string + strlen(token_string), "\n");
			fprintf(fd_tokens_db, "%s", token_string);
		}
		else
			sendToClient(client_fd, "ERROR&User already in game");
	}
	else{
		sendToClient(client_fd, "ERROR&Invalid username or password");
		char name[BUFFER_SIZE];
    sprintf(name, "ERROR! %d inserted invalid credentials", client_fd);
		if (DEBUG_MODE==1){
			pthread_t logFile;
			if (pthread_create(&logFile, NULL, serverLog, name) == 0)
				pthread_join(logFile, NULL);
			else
				perror("Thread creation error!\n");
		}
    writeLog(logFile, name);
	}
  fclose(fd_users_db);
	fclose(fd_tokens_db);
	rot13_crypt();
}


int checkToken(char *buffer) {
	int found = 0;
	FILE* fd_tokens_db = fopen("tokens.txt", "ab+");
	char *line = (char*) newMalloc(BUFFER_SIZE * sizeof(char));
	while(fscanf(fd_tokens_db,"%s",line) != EOF) {
		if(strcmp(line, buffer) == 0){
			found = 1;
			break;
		}
	}
	fclose(fd_tokens_db);
	return found;
}

void sendEffect(player_p plyr, char *effect) {
	if (map[plyr->x][plyr->y].type == CELL_WIN) {
		sprintf(effect, "GAMEOVER&%d&%d&%d",plyr->x,plyr->y,plyr->cod);
		sendToClient(plyr->cod, effect);
	}
	else
		sendToClient(plyr->cod, effect);
}

void sendGameOver(player_p plyr, char *effect){
	sprintf(effect, "GAMEOVER&%d&%d&%d",plyr->x,plyr->y,plyr->cod);
	sendToClient(plyr->cod, effect);
}


void movePlayer(player_p plyr, int x, int y) {
	pthread_mutex_lock(&g_map_mutex);
	if (x==-1)
		x=0;
	else
		if (x==MAP_SIZE)
			x=MAP_SIZE-1;
	if (y==-1)
		y=0;
	else
		if (y==MAP_SIZE)
			y=MAP_SIZE-1;
	int player_cod;
	player_p plyr2;
	char ret[BUFFER_SIZE];
	if(plyr->x == x && plyr->y == y) {
		sprintf(ret, "MOVED&%d&%d&%s", plyr->x, plyr->y, "Map limit reached");
		sendEffect(plyr, ret);
	}
	else {
		switch(map[x][y].type){
			case WALL:
				map[x][y].discovered = 1;
				sprintf(ret, "MOVED&%d&%d&%s", plyr->x, plyr->y,"There's a wall");
				sendEffect(plyr, ret);
				broadcast_wall_list();
				break;
			case CORRIDOR:
				map[plyr->x][plyr->y].type = CORRIDOR;
				plyr->x = x;
				plyr->y = y;
				map[x][y].type = PLAYER + plyr->cod;
				sprintf(ret, "MOVED&%d&%d&%s", plyr->x, plyr->y, "Nothing here");
				sendEffect(plyr, ret);
				broadcast_player_position(plyr);
				break;
			case CELL_WIN:
				plyr->x = x;
				plyr->y = y;
				map[x][y].type = PLAYER + plyr->cod;
				plyr->inGame=0;
				sprintf(ret, "MOVED&%d&%d&%s", plyr->x, plyr->y, "You win!");
				sendEffect(plyr, ret);
				broadcast_player_position(plyr);
				sprintf(ret, "GAMEOVER&%d&%d&%d", plyr->x, plyr->y, plyr->cod);   //moved to win cell
				sendEffect(plyr, ret);
				broadcast_gameOver(plyr->x,plyr->y,plyr->cod,plyr);
				break;
			default:
				player_cod = map[x][y].type - PLAYER;
				plyr2 = getPlayerFromPlayerList(playersList, player_cod);
				sprintf(ret, "MOVED&%d&%d&%s", plyr->x, plyr->y, "There's another player here!");
				sendEffect(plyr, ret);
				break;
		}
	}
	pthread_mutex_unlock(&g_map_mutex);
}

void deletePlayer(int client_fd) {
	player_p plyr = getPlayerFromPlayerList(playersList, client_fd);
	if(plyr){
		map[plyr->x][plyr->y].type = CORRIDOR;
		playerList_p p = NULL;
  	removeFromPlayerList(&playersList, &p, client_fd);
    broadcast_player_left(client_fd);
    char cmd[BUFFER_SIZE];
  	sprintf(cmd, "sed -e '/^%s:.*/d' %s > %s", plyr->name, "tokens.txt", "tokens.tmp");
    system(cmd);
    system("mv tokens.tmp tokens.txt");
    char name[BUFFER_SIZE];
    sprintf(name, "%d removed from game map.", client_fd);
		if (DEBUG_MODE==1){
			pthread_t logFile;
			if (pthread_create(&logFile, NULL, serverLog, name) == 0)
				pthread_join(logFile, NULL);
			else
				perror("Thread creation error!\n");
		}
    writeLog(logFile, name);
	}
}

void spawnUser(char *buffer, int client_fd) {
	int i = 0, isOnline, x, y;
  char *msg;
	msg = strtok(buffer, "&");
	char *line = (char*) newMalloc(BUFFER_SIZE * sizeof(char));
  char *name = (char*) newMalloc(BUFFER_SIZE * sizeof(char));
  char token_string[BUFFER_SIZE];
  while(msg != NULL){
  	if(i == 1){
			sprintf(token_string, "%s", msg);
			sprintf(name, "%s", msg);
    }
		else if(i == 2)
			sprintf(token_string + strlen(token_string),":%s", msg);
		else if(i == 3)
			isOnline = atoi(msg);
		else if(i == 4)
			x = atoi(msg);
		else if(i == 5)
			y = atoi(msg);
		i++;
		msg = strtok(NULL, "&");
  }
  if(!checkToken(token_string)){
    sendToClient(client_fd, "ERROR&Invalid access token");
    return;
  }
  if((x == 0 && y == 0) ||( x == MAP_SIZE-1 && y == MAP_SIZE-1)) {
   	sendToClient(client_fd, "ERROR&Invalid position");
   	return;
  }
  pthread_mutex_lock(&g_map_mutex);
  if(map[x][y].type != CORRIDOR) {
    pthread_mutex_unlock(&g_map_mutex);
    sendToClient(client_fd, "ERROR&Invalid position");
    return;
  }
	pthread_mutex_unlock(&g_map_mutex);
  player_p plyr = getPlayerFromPlayerList(playersList, client_fd);
  if(plyr == NULL){
  	plyr = newMalloc(sizeof(player));
  	insertTopPlayerList(&playersList, plyr);
  }
  plyr->name = name;
  plyr->cod = client_fd;
  plyr->inGame = isOnline;
  plyr->x = x;
	plyr->y = y;
  pthread_mutex_lock(&g_map_mutex);
	map[x][y].type = PLAYER + plyr->cod;
  sprintf(token_string, "MOVED&%d&%d&%s", plyr->x, plyr->y, "User spawned");
  sendToClient(client_fd, token_string);

  broadcast_player_position(plyr);
  broadcast_new_player(plyr);
	broadcast_win_cell();

  send_player_list_to(plyr);
  send_wall_list_to(plyr);

  char logg[BUFFER_SIZE];
  sprintf(logg, "%d added on game map.", client_fd);
	if (DEBUG_MODE==1){
		pthread_t logFile;
		if (pthread_create(&logFile, NULL, serverLog, logg) == 0)
			pthread_join(logFile, NULL);
		else
			perror("Thread creation error!\n");
	}
  writeLog(logFile, logg);
  pthread_mutex_unlock(&g_map_mutex);
}


void *gestisci(void *arg) {
  char *buffer = newMalloc(BUFFER_SIZE * sizeof(char));
  int client_fd = *((int*) arg);
	ClientAccount* account = find_or_create_account(client_fd);
	int ret = read(client_fd, buffer, BUFFER_SIZE);
  while(ret){
   	buffer[ret] = '\0';
  	if(ret != 0){
      if(strstr(buffer, "REGISTER&") != NULL)
				createUser(buffer, client_fd);
      else if (strstr(buffer, "LOGIN&") != NULL)
       	loginUser(buffer, client_fd);
      else if (strstr(buffer, "SPAWN&") != NULL){
				player_p plyr = getPlayerFromPlayerList(playersList, client_fd);
        if(plyr == NULL){
    			plyr = newMalloc(sizeof(player));
    			insertTopPlayerList(&playersList, plyr);
    		}
   			plyr->cod = client_fd;
	      spawnUser(buffer, client_fd);
      }
			else if (strstr(buffer, "MOVE&") != NULL){
      	player_p plyr = getPlayerFromPlayerList(playersList, client_fd);
      	if(plyr != NULL){
      	  char *msg = strtok(buffer, "&");
      	  int i = 0, x, y;
      	 	while(msg != NULL) {
      	  	if (i == 1)
      	  		x = atoi(msg);
      	  	else if (i == 2)
      	  		y = atoi(msg);
      	  	i++;
						msg = strtok(NULL, "&");
      	  }
      	  movePlayer(plyr, x, y);
      	}
				else
      	  sendToClient(client_fd, "ERROR&Non stai attualmente giocando!");
      }
			else if (strstr(buffer, "LOGOUT&") != NULL){
     	  sendToClient(client_fd, "INFO&User logged out");
      	break;
     	}
			else if (strstr(buffer, "WELCOME&") != NULL)
				sendToClient(client_fd, "WELCOME&");
     	else if(strstr(buffer, "Y") != NULL){
				//da ignorare
     	}
			else if(strstr(buffer, "JOIN&") != NULL){
				player_p plyr = getPlayerFromPlayerList(playersList, client_fd);
       	if(plyr == NULL){
    			plyr = newMalloc(sizeof(player));
    			insertTopPlayerList(&playersList, plyr);
    		}
        plyr->cod = client_fd;
				sendToClient(client_fd, "JOINED&");
     	}
			else
     	  sendToClient(client_fd, "ERROR&Comando non valido");
    }
    ret = read(client_fd, buffer, BUFFER_SIZE);
  }
  deletePlayer(client_fd);
  bzero(buffer, BUFFER_SIZE);
	close(client_fd);
  free(buffer);
	pthread_exit(0);
}


unsigned int randomInt(unsigned int min, unsigned int max) {
    int r;
    const unsigned int range = 1 + max - min;
    const unsigned int buckets = RAND_MAX / range;
    const unsigned int limit = buckets * range;
    do {
        r = rand();
    } while (r >= limit);
    return min + (r / buckets);
}


void freeMap() {
	if(map != NULL) {
		int i;
		for(i = 0; i < MAP_SIZE+1; i++)
			free(map[i]);
		free(map);
	}
}

void createMap() {
	freeMap();
	int i = 0, j = 0;
	map = (gameMap**) newMalloc(sizeof(gameMap*)*MAP_SIZE+1);
	if(map){
		for(i = 0; i < MAP_SIZE; i++)
			map[i] = newMalloc(sizeof(gameMap)*MAP_SIZE);
	}
	map[MAP_SIZE] = newMalloc(sizeof(gameMap)*2);
	for(i = 0; i < MAP_SIZE; i++){
		for(j = 0; j < MAP_SIZE; j++){
			map[i][j].type = CORRIDOR;
			map[i][j].discovered = 0;
		}
	}
	for(i = 0; i < MAX_WALLS; i++) {
		int x = randomInt(0, MAP_SIZE-1);
		int y = randomInt(0, MAP_SIZE-1);
		while((x == 0 && y == 0) || (x == MAP_SIZE-1 && y == MAP_SIZE-1)){
			x = randomInt(0, MAP_SIZE-1);
			y = randomInt(0, MAP_SIZE-1);
		}
		map[x][y].type = WALL;
	}
	int x = randomInt(0, MAP_SIZE-1);
	int y = randomInt(0, MAP_SIZE-1);
	while((x == 0 && y == 0) || (x == MAP_SIZE-1 && y == MAP_SIZE-1)){
		x = randomInt(0, MAP_SIZE-1);
		y = randomInt(0, MAP_SIZE-1);
	}
	map[x][y].type = CELL_WIN;
	char name[BUFFER_SIZE];
  sprintf(name, "New session created.");
	if (DEBUG_MODE==1){
		pthread_t logFile;
		if (pthread_create(&logFile, NULL, serverLog, name) == 0)
			pthread_join(logFile, NULL);
	  else
			perror("Thread creation error!\n");
	}
  writeLog(logFile, name);
}

void* handle_selective_broadcast(void *args) {
	broadcast_data *data = args;
	if(data != NULL){
		playerList_p tmp = playersList;
		while(tmp != NULL){
			if(tmp->player->inGame == data->inGame && tmp->player->cod != data->cod)
				sendToClient(tmp->player->cod, data->buffer);
			tmp = tmp->next;
		}
		free(data->buffer);
		free(data);
	}
  pthread_exit(NULL);
}


void broadcast_player_position(player_p plyr) {
	if (playersList != NULL){
		broadcast_data *data = newMalloc(sizeof(broadcast_data));
		char *ret = (char*) newMalloc(sizeof(char)*BUFFER_SIZE);
		sprintf(ret, "USERS&%s&%d&%d&%d", plyr->name, plyr->cod, plyr->x, plyr->y);
		data->inGame = plyr->inGame;
		data->cod = plyr->cod;
		data->buffer = ret;

		pthread_t tid;

		if (pthread_create(&tid, NULL, handle_selective_broadcast, (void *) data) == 0)
			pthread_detach(tid);
		else
    	perror("Thread creation error!\n");
	}
}

void* handle_broadcast(void *args) {
	char *ret = args;
	if(ret != NULL){
		playerList_p tmp = playersList;
		while(tmp != NULL){
			sendToClient(tmp->player->cod, ret);
			tmp = tmp->next;
		}
		free(ret);
	}
	pthread_exit(NULL);
}


void broadcast_gameOver(int x, int y,int cod, player_p plyr) {
  int *seconds = newMalloc(sizeof(int));
	if (playersList != NULL) {
		char name[BUFFER_SIZE];
		char *ret = (char*) newMalloc(sizeof(char)*BUFFER_SIZE);
		sprintf(ret, "GAMEOVER&%d&%d&%d",x,y,cod);
		sprintf(name, "GameOver");
		if (DEBUG_MODE==1){
			pthread_t logFile;
			if (pthread_create(&logFile, NULL, serverLog, name) == 0)
				pthread_join(logFile, NULL);
			else
				perror("Thread creation error!\n");
		}
    writeLog(logFile, name);

		pthread_t tid;

		if (pthread_create(&tid, NULL, handle_broadcast, (void *) ret) == 0)
			pthread_join(tid, NULL);
		else
      perror("Thread creation error!\n");
	}
	*seconds = RESPAWN_TIMEOUT;
  if (pthread_create(&timer_tid, NULL, g_start_timer, (void *) seconds) == 0)
    	pthread_detach(timer_tid);
  else
      perror("Thread creation error!\n");
}


void broadcast_player_left(int client_fd) {
	if (playersList != NULL) {
		char *ret = (char*) newMalloc(sizeof(char)*BUFFER_SIZE);
		sprintf(ret, "LEFT&%d", client_fd);

		pthread_t tid;

		if (pthread_create(&tid, NULL, handle_broadcast, (void *) ret) == 0)
			pthread_detach(tid);
		else
      perror("Thread creation error!\n");
	}
}


void broadcast_win_cell(){
	if (playersList != NULL){
		char *ret = (char*) newMalloc(sizeof(char)*BUFFER_SIZE);
		sprintf(ret, "WIN");
		int x, y;
		for(x = 0; x < MAP_SIZE; x++) {
			for(y = 0; y < MAP_SIZE; y++) {
				if(map[x][y].type == CELL_WIN)
					sprintf(ret + strlen(ret), "&%d&%d", x, y);
			}
		}

		pthread_t tid;

		if (pthread_create(&tid, NULL, handle_broadcast, (void *) ret) == 0)
			pthread_detach(tid);
		else
      perror("Thread creation error!\n");
	}
}

void broadcast_wall_list() {
	if (playersList != NULL){
		char *ret = (char*) newMalloc(sizeof(char)*BUFFER_SIZE);
		sprintf(ret, "WALLS");
		int x, y;
		for(x = 0; x < MAP_SIZE; x++) {
			for(y = 0; y < MAP_SIZE; y++) {
				if(map[x][y].type == WALL) {
					if(map[x][y].discovered)
						sprintf(ret + strlen(ret), "&%d&%d", x, y);
				}
			}
		}

		pthread_t tid;

		if (pthread_create(&tid, NULL, handle_broadcast, (void *) ret) == 0)
			pthread_detach(tid);
	  else
      perror("Thread creation error!\n");
	}
}

void send_player_list_to(player_p plyr) {
	if (playersList != NULL){
		playerList_p tmp = playersList;
		while(tmp != NULL){
			if(tmp->player->inGame == plyr->inGame && tmp->player->cod != plyr->cod) {
				char *ret = (char*) newMalloc(sizeof(char)*BUFFER_SIZE);
				sprintf(ret, "USERS&%s&%d&%d&%d", tmp->player->name, tmp->player->cod, tmp->player->x, tmp->player->y);
				sendToClient(plyr->cod, ret);
			}
			tmp = tmp->next;
		}
	}
}


void send_wall_list_to(player_p plyr) {
	char *ret = (char*) newMalloc(sizeof(char)*BUFFER_SIZE);
	sprintf(ret, "WALLS");
	int x, y;
	for(x = 0; x < MAP_SIZE; x++) {
		for(y = 0; y < MAP_SIZE; y++) {
			if(map[x][y].type == WALL) {
				if(map[x][y].discovered)
					sprintf(ret + strlen(ret), "&%d&%d", x, y);
			}
		}
	}
	sendToClient(plyr->cod, ret);
}


void broadcast_new_player(player_p plyr) {
	if (playersList != NULL){
		char *ret = (char*) newMalloc(sizeof(char)*BUFFER_SIZE);
		sprintf(ret, "PLAYER&%s&%d&%d", plyr->name, plyr->cod, plyr->inGame);

		pthread_t tid;

		if (pthread_create(&tid, NULL, handle_broadcast, (void *) ret) == 0)
			pthread_detach(tid);
		else
    	perror("Thread creation error!\n");
	}
}

void broadcast_new_game() {
	if (playersList != NULL){
		char *ret = (char*) newMalloc(sizeof(char)*BUFFER_SIZE);
		sprintf(ret, "JOINED&");

		pthread_t tid;

		if (pthread_create(&tid, NULL, handle_broadcast, (void *) ret) == 0)
			pthread_join(tid, NULL);
		else
      perror("Thread creation error!\n");
	}
}


void broadcast_timer() {
	if (playersList != NULL){
		char *ret = (char*) newMalloc(sizeof(char)*BUFFER_SIZE);
		sprintf(ret, "TIMEOUT&%d", seconds);

		pthread_t tid;

		if (pthread_create(&tid, NULL, handle_broadcast, (void *) ret) == 0)
			pthread_detach(tid);
		else
      perror("Thread creation error!\n");
	}
}


void* g_start_timer(void *args) {
    seconds = *((int*)args);
    while(seconds > 0){
    	broadcast_timer();
    	sleep(1);
    	seconds--;
    }
		broadcast_new_game();

		freePlayerList(&playersList);

		createMap();

    free(args);
    pthread_exit(NULL);
}


void setupServer() {
	srand(time(NULL));
	logFile = createLog();
	if(logFile == NULL){
		perror("\nLogfile init failed\n");
  	exit(-1);
	}
	if (pthread_mutex_init(&g_list_mutex, NULL) != 0) {
    perror("\nMutex list init failed\n");
    exit(-1);
  }
  if (pthread_mutex_init(&g_map_mutex, NULL) != 0) {
    perror("\nMutex map init failed\n");
    exit(-1);
  }
  g_account_list = NULL;
	createMap();
	playersList = NULL;
}


void *serverLog(void *args){
	char *message=(char*)args;
	serverOut("%s\n",message);
}

void rot13_crypt(){
	FILE *fd=fopen("users.txt","r+");
	if (fd==NULL){
		system("touch users.txt");
		perror(BOLDRED "\nERROR!" DEFAULT " Can't open file. Please restart server.");
		exit(-1);
	}
	char letter=0;
	while ((letter=fgetc(fd))!=EOF) {
  	fseek(fd, ftell(fd) - 1, SEEK_SET);
		fprintf(fd, "%c", letter+13);
  }
	fclose(fd);
}

void rot13_decrypt(){
	FILE *fd=fopen("users.txt","r+");
	if (fd==NULL){
		system("touch users.txt");
		perror(BOLDRED "\nERRORE!" DEFAULT " Can't open file. Please restart server.");
		exit(-1);
	}
	char letter=0;
	while ((letter=fgetc(fd))!=EOF) {
  	fseek(fd, ftell(fd) - 1, SEEK_SET);
		fprintf(fd, "%c", letter-13);
  }
	fclose(fd);
}


static void handle_interrupt(int signum) {
	if(signum == SIGINT || signum == SIGHUP || signum == SIGQUIT || signum == SIGTERM || signum == SIGKILL){
		signal(signum, SIG_IGN);

		closeLog(logFile);
		system("if [ -f tokens.txt ];then rm tokens.txt;fi");
		freeMap();
		if(playersList != NULL)
			freePlayerList(&playersList);
		if(g_account_list != NULL)
			freeAccountList(&g_account_list);
		perror("Server: Disconnected.");
		exit(3);
	}
}

int main(int argc, char **argv) {
	int port;
	if(argc != 2) {
		printf("Insert port number ");
		getPositiveInt(&port);
	}
	char s[BUFFER_SIZE];
	serverOut("\nToggle debug mode? (y/n): ");
	scanf("%s",s);
	if (s[0]=='y' || s[0]=='Y'){
		serverOut("\nDebug mode ON.\n");
		DEBUG_MODE=1;
	}
	else {
		serverOut("\nDebug mode OFF.\n");
	}
	//Handle interrupt signals
	signal(SIGINT, handle_interrupt);
	signal(SIGHUP, handle_interrupt);
	signal(SIGQUIT, handle_interrupt);
	signal(SIGTERM, handle_interrupt);
	signal(SIGKILL, handle_interrupt);

	//Ignore unexpected client disconnection
	signal(SIGPIPE, SIG_IGN);

	setupServer();

	//Start server tcp

    int server_fd, client_fd;
    pthread_t tid;

    struct sockaddr_in address;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
		if (argc != 2)
			address.sin_port=htons(port);
    else
			address.sin_port = htons(atoi(argv[1]));

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
    	perror("\nSocket creation error\n");
    	exit(-1);
    }

    if (bind(server_fd, (struct sockaddr*) &address, sizeof(address)) != 0) {
    	perror("\nBind socket error\n");
    	exit(-1);
    }

    if (listen(server_fd, SOMAXCONN) != 0) {
    	perror("\nlisten socket error\n");
    	exit(-1);
    }

    int c = sizeof(struct sockaddr_in);
    while(1){
        client_fd = accept(server_fd, (struct sockaddr*) &address, (socklen_t*)&c);
        if (client_fd < 0) {
            continue;
        }
        char *ip = inet_ntoa(address.sin_addr);
        char name[BUFFER_SIZE];
        sprintf(name, "New connection from: %s, account code: %d", ip, client_fd);
				if (DEBUG_MODE==1){
					pthread_t logFile;
					if (pthread_create(&logFile, NULL, serverLog, name) == 0)
						pthread_join(logFile, NULL);
					else
						perror("Thread creation error!\n");
				}
        writeLog(logFile, name);

        if (pthread_create(&tid, NULL, gestisci, (void *) &client_fd) == 0) {
        	pthread_detach(tid);
        } else {
        	perror("Thread creation error!\n");
        }
    }

    return 0;
}
