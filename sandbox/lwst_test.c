#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <syslog.h>
#include <libwebsockets.h>

static struct libwebsocket_protocols client_protocols[];

struct conn_info_t{
    struct libwebsocket *wsi;
    struct libwebsocket_context* context;
    volatile uint8_t *exit;
};


static int callback_client(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len)
{
    char *h = (char *)user;
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            fprintf(stderr, "CLIENT: LWS_CALLBACK_CLIENT_ESTABLISHED:%s\n", h);
            strcpy(h,"yayaya");
            libwebsocket_callback_on_writable(context, wsi);
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            fprintf(stderr, "CLIENT: LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n");
            break;

        case LWS_CALLBACK_CLOSED:
            fprintf(stderr, "CLIENT: LWS_CALLBACK_CLOSED\n");
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            printf("LWS_CALLBACK_CLIENT_WRITEABLE:%s\n", h);
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            break;
        default:
            break;
    }
    return 0;
}


void signal_connect(struct libwebsocket_context *context, volatile uint8_t *exit)
{
    fprintf(stderr, "SIGNALING: waiting for connect...\n");
    int n = 0;
    while( n >= 0 && !(*exit)){
        n = libwebsocket_service(context, 20);
    }
    fprintf(stderr, "SIGNALING: close connect\n\n");
}


struct conn_info_t* signal_initial(const char *address, int port, struct libwebsocket_protocols protocols[], char *protocol_name)
{
    struct libwebsocket *wsi;
    int use_ssl = 0;
    const char *iface = NULL;
    int ietf_version = -1; 
    int opts = 0;

    /*syslog setting*/

    int syslog_options = LOG_PID | LOG_PERROR; 
    int debug_level = 7;
    setlogmask(LOG_UPTO (LOG_DEBUG));
    openlog("lwsts", syslog_options, LOG_DAEMON);
    lws_set_log_level(debug_level, lwsl_emit_syslog);
    /*end syslog setting*/
    
    struct libwebsocket_context *context; 
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.iface = iface;
    info.protocols = protocols;
    info.extensions = libwebsocket_get_internal_extensions();

    info.gid = -1;
    info.uid = -1;
    info.options = opts;
    char *sttt = (char *) calloc(1, 1024);
    strcpy(sttt, "hahahah");
    struct conn_info_t* SDP_conn = (struct conn_info_t *)calloc(1, sizeof(struct conn_info_t)); 
    context = libwebsocket_create_context(&info);
    if (context == NULL) {
        lwsl_err("libwebsocket_create_context failed\n");
        goto bail;
    }

    SDP_conn->context = context;

    wsi = libwebsocket_client_connect_extended(context, address, port, use_ssl, "/", 
            address, address, protocol_name, ietf_version, (void *)sttt);
    if (wsi == NULL)
    {
        fprintf(stderr, "libwebsocket_client_connect failed\n");
        goto bail;
    }
    SDP_conn->wsi = wsi;
    return SDP_conn;
bail:
    fprintf(stderr, "Exit\n");
    libwebsocket_context_destroy(context); 
    closelog();
    return 0;

}



static struct libwebsocket_protocols client_protocols[] = {
    {
        "client-protocol",
        callback_client,
        512,
        10,
    },
    { NULL, NULL, 0, 0 }
};


int main(int argc, char *argv[]){

    // initial signaling
    const char *address = "140.112.90.37";
    int port = 7681;
    struct conn_info_t *signal_conn = signal_initial(address, port, client_protocols, "client-protocol");
    struct libwebsocket_context *context = signal_conn->context;
    volatile uint8_t client_exit = 0;
    signal_conn->exit = &client_exit;
    signal_connect(context, signal_conn->exit);

    return 0;
}

