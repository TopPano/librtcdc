#include "../signaling.h"
#include <libwebsockets.h>

#define SIG_SERVER_IP "140.112.90.37"
#define SIG_SERVER_PORT 7681
#define REPOS_PATH "/home/uniray/warehouse"


/* 
 * whenever the fileserver is online, reigter to the signaling server 
 * after registering, waiting for the connection of users
 */

static volatile int register_exit;

static int callback_SDP(struct libwebsocket_context *context, 
        struct libwebsocket *wsi, 
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len){
    SESSIONstate *session_state = (SESSIONstate *)user;
    static struct signal_session_data_t *sent_session_data = NULL;
    static struct signal_session_data_t *recvd_session_data = NULL;
    int result;
    switch (reason){
        case LWS_CALLBACK_CLIENT_ESTABLISHED:    
            fprintf(stderr, "FILESERVER: LWS_CALLBACK_CLIENT_ESTABLISHED\n");
            // start register
            *session_state = FILESERVER_INITIAL;
            libwebsocket_callback_on_writable(context, wsi);
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            fprintf(stderr, "FILESERVER: LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n");
            break;
        
        case LWS_CALLBACK_CLOSED:
            fprintf(stderr, "FILESERVER: LWS_CALLBACK_CLOSED\n");
            register_exit = 1 ;
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            //send msg: SERVER_REGISTER
            sent_session_data = (struct signal_session_data_t *)calloc(1, sizeof(struct signal_session_data_t));
            switch(*session_state){ 
                case(FILESERVER_INITIAL):
                    sent_session_data->type = FILESERVER_REGISTER_t;
                    memset(sent_session_data->fileserver_dns, '\0', NAMESIZE);
                    fprintf(stderr, "FILESERVER: send FILESERVER_REGISTER_t\n");
                    result = libwebsocket_write(wsi, (void *)sent_session_data, sizeof(struct signal_session_data_t), LWS_WRITE_TEXT);
                    if(result == -1)
                        fprintf(stderr, "libwebsocket_write error\n");
                    free(sent_session_data);
                    break;
              default:
                    break;
                    }
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            recvd_session_data = (struct signal_session_data_t *)in;
            if( *session_state == FILESERVER_INITIAL){
                switch (recvd_session_data->type){
                    case FILESERVER_REGISTER_OK_t:
                        *session_state = FILESERVER_READY;
                        fprintf(stderr, "FILESERVER: ready\n");
                        break;
                    default:
                        break;
                }
            }
            else if ( *session_state == FILESERVER_READY){
                switch (recvd_session_data->type){
                    case CLIENT_SDP_t:
                        /*
                         * whenever receive client SDP,
                         * parse it and generate FILESERVER SDP
                         * the send it back
                         */
                        break;
                    default:
                        break;
                }
            }
            break;
        default:
            // timeout and not receive SERVER_REGISTER_OK? do what?
            break;
    }
}


static int callback_SDP2(struct libwebsocket_context *context, 
        struct libwebsocket *wsi, 
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len){
    // check fileserver_isRegister?
    switch (reason){
        case LWS_CALLBACK_CLIENT_ESTABLISHED:    
            fprintf(stderr, "SDP: LWS_CALLBACK_CLIENT_ESTABLISHED\n");
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            fprintf(stderr, "SDP: LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n");
            break;
        
        case LWS_CALLBACK_CLOSED:
            fprintf(stderr, "SDP: LWS_CALLBACK_CLOSED\n");
            break;


        case LWS_CALLBACK_CLIENT_WRITEABLE:

/*          
 *          send the created SDP candidate back wsi, client_dns
 */
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
/*
 *          wait for receiving SDP
 *
 *          switch (status){
 *          case CLIENT_SDP:
 *              verifying recved SDP candidate?
 *              create rtcdc sdp & candidate and send back
 *              on_writable
 *          }
 */

            break;
        default:
            // timeout and not receive SERVER_REGISTER_OK? do what?
            break;
    }


}


static struct libwebsocket_protocols protocols[] = {
    {
        "SDP-protocol",
        callback_SDP,
        sizeof(struct signal_session_data_t),
        10,
    },
    { NULL, NULL, 0, 0 }
};


void fileserver_setup(const char *signalserver_IP, int signalserver_port)
{
    // initialize libwebsockets
    struct signal_conn_info *conn = signal_initial(signalserver_IP, signalserver_port, protocols);
    
    register_exit = 0;
    // signal_connect()
    signal_connect(conn, &register_exit);

    // blocked until the receive SERVER_REGISTER_OK
}


int main(){
    fileserver_setup(SIG_SERVER_IP, SIG_SERVER_PORT);
    return 0;
}
