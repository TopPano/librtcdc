#include "signaling.h"
#include <stdio.h>


struct conn_info *signal_conn;
static volatile int fileserver_exit;


static int callback_fileserver(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len)
{
    struct signal_session_data_t *sent_session_data;
    struct signal_session_data_t *recvd_session_data = NULL;
    SESSIONstate *session_state = (SESSIONstate *)user;
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
                        fprintf(stderr, "FILE_SERVER: RECEIVE CLIENT_SDP_t\nSDP: %s\ncandidates: %s\n", recvd_session_data->SDP, recvd_session_data->candidates);
                        
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
    // initial signaling
    const char *address = "140.112.90.37";
    int port = 7681;
    signal_conn = signal_initial(address, port, fileserver_protocols, "fileserver-protocol");
    struct libwebsocket_context *context = signal_conn->context;
    fileserver_exit = 0;
    signal_connect(context, &fileserver_exit);

    return 0;
}

