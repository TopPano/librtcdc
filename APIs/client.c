#include <stdio.h>
#include <rtcdc.h>
#include "signaling.h"

typedef struct rtcdc_peer_connection rtcdc_peer_connection;
typedef struct rtcdc_data_channel rtcdc_data_channel;

struct conn_info *signal_conn;
static volatile int client_exit;
static rtcdc_peer_connection* offerer;


void on_message(rtcdc_data_channel* dc, int datatype, void* data, size_t len, void* user_data){
}

void on_close_channel(){
    fprintf(stderr, "close channel!\n");
}


void on_channel(rtcdc_peer_connection *peer, rtcdc_data_channel* dc, void* user_data){
    dc->on_message = on_message;
    fprintf(stderr, "on channel!!\n");
}


void on_candidate(rtcdc_peer_connection *peer, char *candidate, void *user_data ){
    char *endl = "\n";
    char *candidates = (char *)user_data;
    candidates = strcat(candidates, candidate);
    candidates = strcat(candidates, endl);
}


void on_connect(){
    fprintf(stderr, "on connect!\n");
}


static int callback_client(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len)
{
    struct signal_session_data_t *sent_session_data;
    struct signal_session_data_t *recvd_session_data = NULL;
    SESSIONstate *session_state = (SESSIONstate *)user;
    int result;
    char *client_SDP, *client_candidates;
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            fprintf(stderr, "CLIENT: LWS_CALLBACK_CLIENT_ESTABLISHED\n");
            *session_state = CLIENT_INITIAL;
            libwebsocket_callback_on_writable(context, wsi);
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            fprintf(stderr, "CLIENT: LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n");
            break;

        case LWS_CALLBACK_CLOSED:
            fprintf(stderr, "CLIENT: LWS_CALLBACK_CLOSED\n");
            client_exit = 1;
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            switch(*session_state){ 
                case(CLIENT_INITIAL):
                    // gen client SDP & candidate and send to the signal server
                    client_candidates = (char *)calloc(1, DATASIZE*sizeof(char));
                    offerer = rtcdc_create_peer_connection((void*)on_channel, (void*)on_candidate, (void *)on_connect, "stun.services.mozilla.com", 0, (void *)client_candidates);
                    client_SDP = rtcdc_generate_offer_sdp(offerer);
                    
                    sent_session_data = (struct signal_session_data_t *)calloc(1, sizeof(struct signal_session_data_t));
                    sent_session_data->type = CLIENT_SDP_t;
                    memset(sent_session_data->fileserver_dns, 0, NAMESIZE);
                    memset(sent_session_data->client_dns, 0, NAMESIZE);
                    strcpy(sent_session_data->SDP, client_SDP);
                    strcpy(sent_session_data->candidates, client_candidates);

                    result = libwebsocket_write(wsi, (char *) sent_session_data, sizeof(struct signal_session_data_t), LWS_WRITE_TEXT);
                    if(result < 0)
                        fprintf(stderr, "libwebsocket_write error\n");

                    free(sent_session_data);
                    *session_state = CLIENT_WAIT;
                    break;
                default:
                    break;
            }
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            recvd_session_data = (struct signal_session_data_t *)in;
            if(*session_state == CLIENT_WAIT){ 
                switch(recvd_session_data->type){
                    case FILESERVER_SDP_t:
                        fprintf(stderr, "CLIENT: RECEIVE FILESERVER_SDP_t\n");
                        // receiving file server sdp and parse it
                        // if success exchange sdp, then close libwebsocket
                        client_exit = 1;
                        break;
                    default:
                        break;
                }
            }

            break;
        default:
            break;
    }
    return 0;
}


static struct libwebsocket_protocols client_protocols[] = {
    {
        "client-protocol",
        callback_client,
        sizeof(struct signal_session_data_t),
        10,
    },
    { NULL, NULL, 0, 0 }
};


int main(int argc, char *argv[]){

    // initial signaling
    const char *address = "140.112.90.37";
    int port = 7681;
    signal_conn = signal_initial(address, port, client_protocols, "client-protocol");
    struct libwebsocket_context *context = signal_conn->context;
    client_exit = 0;
    // generate exchange and parse sdp & candidates
    offerer = NULL;
    signal_connect(context, &client_exit);
    
    // TODO: create data channel

    return 0;
}

