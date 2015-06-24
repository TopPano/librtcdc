#include <stdio.h>
#include <rtcdc.h>
#include <uv.h>
#include "signaling.h"

typedef struct rtcdc_peer_connection rtcdc_peer_connection;
typedef struct rtcdc_data_channel rtcdc_data_channel;

struct conn_info *signal_conn;
static volatile int fileserver_exit;
static uv_loop_t *fileserver_loop;

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


void parse_candidates(rtcdc_peer_connection *peer, char *candidates)
{
    char *line;
    while((line = signal_getline(&candidates)) != NULL){    
        rtcdc_parse_candidate_sdp(peer, line);
        // printf("%s\n", line);
    }

}


void rtcdc_connect(uv_work_t *work){
    rtcdc_peer_connection* answerer = (rtcdc_peer_connection*)work->data;
    rtcdc_loop(answerer);
}


static int callback_fileserver(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len)
{
    struct signal_session_data_t *sent_session_data = NULL;
    struct signal_session_data_t *recvd_session_data = NULL;
    SESSIONstate *session_state = (SESSIONstate *)user;

    char *fileserver_SDP = NULL;
    char *fileserver_candidates = NULL; 
    char *client_SDP = NULL;
    char *client_candidates = NULL; 
    rtcdc_peer_connection* answerer = NULL;

    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            fprintf(stderr, "FILE_SERVER: LWS_CALLBACK_CLIENT_ESTABLISHED\n");
            *session_state = FILESERVER_INITIAL;
            libwebsocket_callback_on_writable(context, wsi);
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            fprintf(stderr, "FILE_SERVER: LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n");
            break;

        case LWS_CALLBACK_CLOSED:
            fprintf(stderr, "FILE_SERVER: LWS_CALLBACK_CLOSED\n");
            // TODO when close signaling session, fileserver needs to deregister to signaling server
            fileserver_exit = 1;
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            switch(*session_state){ 
                case(FILESERVER_INITIAL):
                    sent_session_data = (struct signal_session_data_t *)calloc(1, sizeof(struct signal_session_data_t));
                    sent_session_data->type = FILESERVER_REGISTER_t;
                    memset(sent_session_data->fileserver_dns, 0, NAMESIZE);
                    memset(sent_session_data->client_dns, 0, NAMESIZE);
                    memset(sent_session_data->SDP, 0, DATASIZE);
                    memset(sent_session_data->candidates, 0, DATASIZE);
                    libwebsocket_write(wsi, (char *) sent_session_data, sizeof(struct signal_session_data_t), LWS_WRITE_TEXT);
                    free(sent_session_data);
                default:
                    break;
            }
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            recvd_session_data = (struct signal_session_data_t *)in;
            if(*session_state == FILESERVER_INITIAL){ 
                switch(recvd_session_data->type){
                    case FILESERVER_REGISTER_OK_t:
                        fprintf(stderr, "FILE_SERVER: RECEIVE FILESERVER_REGISTER_OK_t\n");
                        *session_state = FILESERVER_READY;
                        break;
                    default:
                        break;
                }
            }
            else if(*session_state == FILESERVER_READY){
                switch(recvd_session_data->type){
                    case CLIENT_SDP_t:

                        // create rtcdc_peer_connection
                        fileserver_candidates = (char *)calloc(1, DATASIZE*sizeof(char));
                        answerer = rtcdc_create_peer_connection((void*)on_channel, (void*)on_candidate, (void *)on_connect, "stun.services.mozilla.com", 0, (void *)fileserver_candidates);

                        // parse client sdp
                        rtcdc_parse_offer_sdp(answerer, recvd_session_data->SDP);
                        parse_candidates(answerer, recvd_session_data->candidates);

                        // gen fileserver sdp and send back
                        fileserver_SDP = rtcdc_generate_offer_sdp(answerer);
                        
                        sent_session_data = (struct signal_session_data_t *)calloc(1, sizeof(struct signal_session_data_t));
                        sent_session_data->type = FILESERVER_SDP_t;
                        strcpy(sent_session_data->fileserver_dns, recvd_session_data->fileserver_dns);
                        strcpy(sent_session_data->client_dns, recvd_session_data->client_dns);
                        strcpy(sent_session_data->SDP, fileserver_SDP);
                        strcpy(sent_session_data->candidates, fileserver_candidates);
                        libwebsocket_write(wsi, (char *) sent_session_data, sizeof(struct signal_session_data_t), LWS_WRITE_TEXT);
                        free(sent_session_data);
                        
                        // libuv rtcdc_loop()
                        uv_work_t *rtcdc_conn_work = (uv_work_t *)calloc(1, sizeof(uv_work_t));
                        rtcdc_conn_work->data = (void *)answerer;
                        uv_queue_work(fileserver_loop, rtcdc_conn_work, rtcdc_connect, NULL);

                        // rtcdc_loop(answerer);

                        break;
                    default:
                        break;
                } // end switch
            } // end if
            break;
        default:
            break;
    }
    return 0;
}


static struct libwebsocket_protocols fileserver_protocols[] = {
    {
        "fileserver-protocol",
        callback_fileserver,
        sizeof(struct signal_session_data_t),
        10,
    },
    { NULL, NULL, 0, 0 }
};


int main(int argc, char *argv[]){
    // initial uv_loop
    fileserver_loop = uv_default_loop();
    // initial signaling
    const char *address = "140.112.90.37";
    int port = 7681;
    signal_conn = signal_initial(address, port, fileserver_protocols, "fileserver-protocol");
    struct libwebsocket_context *context = signal_conn->context;
    fileserver_exit = 0;
    signal_connect(context, &fileserver_exit);

    return 0;
}

