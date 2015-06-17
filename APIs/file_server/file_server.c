#include "../signaling.h"
#include <libwebsockets.h>

/* 
 * whenever the fileserver is online, reigter to the signaling server 
 * after registering, waiting for the connection of users
 */

static int fileserver_isRegister;
static int register_exit;



static int callback_register(struct libwebsocket_context *context, 
        struct libwebsocket *wsi, 
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len){
    switch (reason){
        case LWS_CALLBACK_CLIENT_ESTABLISHED:    
            fprintf(stderr, "register: LWS_CALLBACK_CLIENT_ESTABLISHED\n");
            // start register
            // on_writable
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            fprintf(stderr, "register: LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n");
            break;
        
        case LWS_CALLBACK_CLOSED:
            fprintf(stderr, "register: LWS_CALLBACK_CLOSED\n");
            break;


        case LWS_CALLBACK_CLIENT_WRITEABLE:

        //send msg: SERVER_REGISTER
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
/*
 *          switch (status){
 *          case SERVER_REGISTER_OK:
 *          set fileserver_isRegister = 1; 
 *          register_force_exit = 1;
 *          close the signal register
 *          }
 */

            break;
        default:
            // timeout and not receive SERVER_REGISTER_OK? do what?
            break;
    }
}


static int callback_SDP(struct libwebsocket_context *context, 
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



void fileserver_register(const char *signalserver_IP, int signalserver_port)
{
    // initialize libwebsockets
    // create libwebsocket_context
    
    // signal_initial(signalserver_IP, signalserver_port, protocols);
    
    // signal_connect()
    // blocked until the receive SERVER_REGISTER_OK
}



void fileserver_listen(const char *signalserver_IP, int signalserver_port, const char* repos_path)
{
    // initialize libwebsockets
    // create libwebsocket_context
    
    // signal_initial(signalserver_IP, signalserver_port, protocols);
    
    // signal_connect()

    // blocked until force exit

}

static struct libwebsocket_protocols protocols[] = {
    {
        "http-only",
        callback_http,
        0,
        0,
    },
    {
        "SDP-protocol",
        callback_SDP,
        sizeof(struct signal_SDP_data),
        10,
    },
    {
        "register-protocol",
        callback_register,
        sizeof(struct signal_register_data),
        10,
    },   
    
    { NULL, NULL, 0, 0 }
};


int main(){

}
