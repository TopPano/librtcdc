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
    SIGNALSERVER_RECV_REGISTER,
    CLIENT_INITIAL,
    CLIENT_WAIT,

    CLIENT_OFF,
} SESSIONstate;

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

struct conn_info{
    struct libwebsocket *wsi;
    struct libwebsocket_context* context;
};

struct signal_session_data_t{
    SIGNALDATAtype type;
    char fileserver_dns[NAMESIZE];
    char client_dns[NAMESIZE];
    // rtcdc_peer_connection *; 
    char SDP[DATASIZE];
    char candidates[DATASIZE];
};


struct conn_info* signal_initial(const char *address, int port, struct libwebsocket_protocols protocols[], char *protocol_name);

char *signal_getline(char **string);

void signal_connect(struct libwebsocket_context *context, volatile int *exit);

struct SDP_context* signal_req(struct conn_info* SDP_conn, char* peer_name);

