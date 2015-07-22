#!/bin/bash
rm file_server sy metadata_server
#gcc -lwebsockets -ljansson -o signal_server signal_server.c signaling.h signaling.c $(pkg-config --cflags --libs libmongoc-1.0)
gcc -DDEBUG_META -g -lwebsockets -ljansson -o metadata_server metadata_server.c signaling.h signaling.c $(pkg-config --cflags --libs libmongoc-1.0)
gcc -DDEBUG_FS -lrtcdc -lwebsockets -ljansson signaling.c signaling.h file_server.c -o file_server
gcc -DDEBUG_SY -g -luv -lrtcdc -lwebsockets -ljansson signaling.c signaling.h sy.h sy.c -o sy