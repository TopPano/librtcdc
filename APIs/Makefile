all: metadata_server file_server client

metadata_server: ms/metadata_server.c ms/ms_struct.h common.h
	gcc -DDEBUG_META -g -lwebsockets -ljansson -o metadata_server ms/metadata_server.c `pkg-config --cflags --libs libmongoc-1.0`

file_server: fs/file_server.c sy_lwst.c sy_lwst.h fs/fs_rtcdc.c fs/fs_rtcdc.h common.h sy_rtcdc.h sy_rtcdc.c
	gcc -DDEBUG_FS -DDEBUG_RTCDC -g -lrtcdc -luv -lwebsockets -ljansson -lcrypto sy_lwst.c sy_lwst.h fs/fs_rtcdc.c fs/fs_rtcdc.h fs/file_server.c sy_rtcdc.h sy_rtcdc.c -o file_server

client: cli/sy.c cli/sy.h sy_lwst.c sy_lwst.h cli/cli_rtcdc.c cli/cli_rtcdc.h common.h sy_rtcdc.h sy_rtcdc.c
	gcc -DDEBUG_SY -DDEBUG_RTCDC -g  -lcrypto -l:libuv.so.1 -lrtcdc -lwebsockets -ljansson sy_lwst.c sy_lwst.h sy_rtcdc.c sy_rtcdc.h cli/cli_rtcdc.c cli/cli_rtcdc.h cli/sy.h cli/sy.c -o client


clean:
	rm file_server client metadata_server
