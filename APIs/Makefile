metadata_server: metadata_server.c signaling.c signaling.h
	gcc -DDEBUG_META -g -lwebsockets -ljansson -o metadata_server metadata_server.c signaling.h signaling.c `pkg-config --cflags --libs libmongoc-1.0`

file_server: file_server.c signaling.c signaling.h
	gcc -DDEBUG_FS -g -lrtcdc -lwebsockets -ljansson -lcrypto signaling.c signaling.h file_server.c -o file_server

client: sy.c sy.h signaling.c signaling.h
	gcc -DDEBUG_SY -g  -lcrypto -luv -lrtcdc -lwebsockets -ljansson signaling.c signaling.h sy.h sy.c -o client

all: metadata_server file_server client

clean:
	rm file_server client metadata_server
