#!/bin/bash

cd /home/uniray/Project/librtcdc/src/
make clean
make
sudo rm /usr/local/lib/librtcdc.so
sudo cp librtcdc.so /usr/local/lib/
make clean
cd /home/uniray/Project/librtcdc/examples/file_trans/
rm send
rm recv

gcc -luv -lrtcdc -lpthread -lwebsockets signal.c signal.h send_file.c -o send
gcc -luv -lrtcdc -lpthread -lwebsockets signal.h signal.c recv_file.c -o recv
