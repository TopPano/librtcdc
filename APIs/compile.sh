#!/bin/bash
rm server file_server
gcc -lwebsockets -ljansson -o server signal_server.c signaling.h signaling.c $(pkg-config --cflags --libs libmongoc-1.0)
gcc -DDEBUG_META -lwebsockets -ljansson -o metadata_server metadata_server.c signaling.h signaling.c $(pkg-config --cflags --libs libmongoc-1.0)
gcc -DDEBUG_FS -lrtcdc -lwebsockets -ljansson signaling.c signaling.h file_server.c -o file_server
gcc -DDEBUG_SY -g -luv -lrtcdc -lwebsockets -ljansson signaling.c signaling.h sy.h sy.c -o client
