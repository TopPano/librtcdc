
all: metadata_server file_server client
metadata_server: metadata_server.c signaling.c signaling.h
	gcc -DDEBUG_META -g -lwebsockets -ljansson -o metadata_server metadata_server.c signaling.h signaling.c `pkg-config --cflags --libs libmongoc-1.0`

file_server: file_server.c signaling.c signaling.h
	gcc -DDEBUG_FS -g -lrtcdc -l:libuv.so -lwebsockets -ljansson -lcrypto signaling.c signaling.h signaling_rtcdc.c signaling_rtcdc.h file_server.c -o file_server

client: sy.c sy.h signaling.c signaling.h signaling_rtcdc.c signaling_rtcdc.h
	gcc -DDEBUG_SY -g  -lcrypto -l:libuv.so -lrtcdc -lwebsockets -ljansson signaling.c signaling.h signaling_rtcdc.c signaling_rtcdc.h sy.h sy.c -o client


clean:
	rm file_server client metadata_server
