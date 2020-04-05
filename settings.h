

//UNIX shell colors

#define DEFAULT        "\e[0m"
#define BOLD           "\e[1m"
#define UNDERLINE      "\e[4m"
#define BLINK          "\e[5m"
#define DEFAULTBLINK   "\e[25m"

//bold and colored
#define BOLDRED        "\e[1m\e[31m"
#define BOLDCYAN       "\e[1m\e[36m"
#define BOLDBLUE       "\e[1m\e[34m"
#define BOLDGREEN      "\e[1m\e[32m"
#define BOLDYELLOW     "\e[1m\e[33m"

//foreground colors
#define FGRED              "\e[31m"
#define FGGREEN            "\e[32m"
#define FGYELLOW           "\e[33m"
#define FGBLUE             "\e[34m"
#define FGMAGENTA          "\e[35m"
#define FGCYAN             "\e[36m"
#define FGGRAY             "\e[90m"
#define FGLIGHTRED         "\e[91m"
#define FGLIGHTGREEN       "\e[92m"
#define FGLIGHYELLOW       "\e[93m"
#define FGLIGHTBLUE        "\e[94m"
#define FGLIGHTMAGENTA     "\e[95m"
#define FGLIGHTCYAN        "\e[96m"
#define FGDEFAULT          "\e[39m"

//background colors
#define BGRED              "\e[41m"
#define BGGREEN            "\e[42m"
#define BGYELLOW           "\e[43m"
#define BGBLUE             "\e[44m"
#define BGMAGENTA          "\e[45m"
#define BGCYAN             "\e[46m"
#define BGGRAY             "\e[100m"
#define BGLIGHTRED         "\e[101m"
#define BGLIGHTGREEN       "\e[102m"
#define BGLIGHTYELLOW      "\e[103m"
#define BGLIGHTBLUE        "\e[104m"
#define BGLIGHTMAGENTA     "\e[105m"
#define BGLIGHTCYAN        "\e[106m"
#define BGWHITE            "\e[107m"
#define BGDEFAULT          "\e[49m"


#define serverOut printf

//////////SETTINGS//////////////////
#define RESPAWN_TIMEOUT 5

#define MAX_WALLS (MAP_SIZE*MAP_SIZE)/4

#define BUFFER_SIZE 512
#define MAP_SIZE 18
////////////////////////////////////


///////////MAP ELEMENTS/////////////
#define CORRIDOR 0
#define WALL 1
#define CELL_WIN 2
#define PLAYER 4
////////////////////////////////////
