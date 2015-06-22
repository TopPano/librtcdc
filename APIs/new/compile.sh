#!/bin/bash
rm server file_server
gcc -lwebsockets -lpthread -o server signal_server.c signal.h signal.c $(pkg-config --cflags --libs libmongoc-1.0)
gcc -luv -lrtcdc -lpthread -lwebsockets signal.c signal.h file_server.c -o file_server
