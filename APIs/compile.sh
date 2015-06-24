#!/bin/bash
rm server file_server
gcc -lwebsockets -lpthread -o server signal_server.c signaling.h signaling.c $(pkg-config --cflags --libs libmongoc-1.0)
gcc -luv -lrtcdc -lwebsockets signaling.c signaling.h file_server.c -o file_server
gcc -lrtcdc -lwebsockets signaling.c signaling.h client.c -o client
