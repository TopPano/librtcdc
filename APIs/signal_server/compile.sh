#!/bin/bash
cd /home/uniray/Project/librtcdc/APIs/signal_server
rm signal_server 
gcc -lwebsockets -lpthread -o signal_server signal_server.c ../signaling.h ../signaling.c $(pkg-config --cflags --libs libmongoc-1.0)
