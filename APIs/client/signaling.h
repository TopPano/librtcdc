#include <libwebsockets.h>
#include <stdio.h>

#define DATASIZE 1024
#define NAMESIZE 128




typedef enum {
    FILESERVER_SDP = 0,
    CLIENT_SDP,
    SERVER_REGISTER,
    SERVER_REGISTER_OK
} SIGNALcode;

struct SDP_context{
    char peer_name[NAMESIZE];
    char SDP[DATASIZE];
    char candidate[DATASIZE];
};

struct signal_conn_info{
    struct libwebsocket *wsi;
    struct libwebsocket_context* context;
};


struct signal_register_data_t{
    SIGNALcode state;
    char peer_name[NAMESIZE];
    char data[DATASIZE];
};


struct signal_SDP_data_t{
    SIGNALcode state;
    char fileserver_dns[NAMESIZE];
    char client_dns[NAMESIZE];

    char peer_name[NAMESIZE];
    char data[DATASIZE];
};

struct req_context{
    char requested_peer_name[NAMESIZE];
    struct libwebsocket *requester_wsi;
};


struct signal_conn_info* signal_initial(const char *address, int porti, struct libwebsocket_protocols[]);

char *signal_getline(char **string);

void* signal_connect(void *pthread_data);

void signal_close(struct signal_conn_info *SDP_conn);

//void signal_send(struct signal_conn_info *SDP_conn, struct session_data__SDP *session_data);

//void signal_send_SDP(struct signal_conn_info *SDP_conn, char *peer_name, char *SDP);

