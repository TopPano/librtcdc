#!/bin/bash
cd /home/uniray/Project/librtcdc/APIs/file_server
rm file_server 
gcc -lwebsockets -lrtcdc -o file_server file_server.c ../signaling.h ../signaling.c 
