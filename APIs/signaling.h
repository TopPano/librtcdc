#include <libwebsockets.h>
#include <stdio.h>

#define DATASIZE 1024
#define NAMESIZE 128


typedef enum {
    FILESERVER_REGISTER_t = 0,
    FILESERVER_REGISTER_OK_t,
    FILESERVER_SDP_t,
    CLIENT_SDP_t,
} SIGNALDATAtype;


typedef enum {
    FILESERVER_INITIAL = 0,
    FILESERVER_READY,
    SIGNALSERVER_READY,
    CLIENT_OFF,
} SESSIONstate;


struct signal_session_data_t{
    SIGNALDATAtype type;
    char fileserver_dns[100];
//    char client_dns[NAMESIZE];

};


struct signal_conn_info{
    struct libwebsocket *wsi;
    struct libwebsocket_context* context;
};


char *signal_getline(char **string);

struct signal_conn_info* signal_initial(const char *address, int port, struct libwebsocket_protocols protocols[]);

void* signal_connect(struct signal_conn_info *conn, volatile int *force_int);

void signal_close(struct signal_conn_info *conn);

