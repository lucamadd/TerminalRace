#! /bin/bash

gcc client.c InputLib.c walls.c player.c -o client -lpthread
gcc server.c player.c log.c InputLib.c -o server -lpthread
