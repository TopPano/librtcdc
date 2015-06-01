#include <libwebsockets.h>
#include <stdio.h>

#define DATASIZE 1024
#define NAMESIZE 128


enum SDP_state {
    SEND_LOCAL_SDP = 0,
    SEND_LOCAL_CANDIDATE,
    RECV_SDP_CONTEXT,
    REQ_SDP_CONTEXT,
};

struct SDP_context{
    char peer_name[NAMESIZE];
    char SDP[DATASIZE];
    char candidate[DATASIZE];
    enum SDP_state state;
};

struct req_context{
    char requested_peer_name[NAMESIZE];
    struct libwebsocket *requester_wsi;
};

struct conn_info__SDP{
    struct libwebsocket *wsi;
    struct libwebsocket_context* context;
};

struct session_data__SDP{
    enum SDP_state state;
    char peer_name[NAMESIZE];
    char data[DATASIZE];
};

struct conn_info__SDP* signal_initial(const char *address, int port);

char *signal_getline(char **string);

void* signal_connect(void *pthread_data);

void signal_close(struct conn_info__SDP *SDP_conn);

void signal_send(struct conn_info__SDP *SDP_conn, struct session_data__SDP *session_data);

void signal_send_SDP(struct conn_info__SDP *SDP_conn, char *peer_name, char *SDP);

void signal_send_candidate(struct conn_info__SDP *SDP_conn, char *peer_name, char *candidate);

struct SDP_context* signal_req(struct conn_info__SDP* SDP_conn, char* peer_name);

