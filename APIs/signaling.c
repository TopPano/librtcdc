#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include "signaling.h"

static struct libwebsocket_context *context;
static volatile int force_exit = 0;
static struct signal_session_data_t* signal_session_data;
static struct SDP_context *recvd_SDP;
pthread_mutex_t mutex;

char *signal_getline(char **string)
{
    char *line = (char *)calloc(1, 128*sizeof(char));
    int str_iter;
    if((*string)[0] == 0)
        return NULL;
    for (str_iter = 0; ; str_iter++)
    {
        if((*string)[str_iter] != '\n')
            line[str_iter] = (*string)[str_iter];
        else
        {    
            *string+=(str_iter+1); 
            break;
        }
    }
    return line;
}

struct conn_info* signal_initial(const char *address, int port, struct libwebsocket_protocols protocols[], char *protocol_name)
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

    struct conn_info* SDP_conn = (struct conn_info *)calloc(1, sizeof(struct conn_info)); 
    context = libwebsocket_create_context(&info);
    if (context == NULL) {
        lwsl_err("libwebsocket_create_context failed\n");
        goto bail;
    }
    SDP_conn->context = context;

    wsi = libwebsocket_client_connect(context, address, port, use_ssl, "/", 
            address, address, protocol_name, ietf_version);
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


void signal_close(struct conn_info* conn)
{
    fprintf(stderr, "close the connect\n");
    struct libwebsocket_context *context = conn->context;
    struct libwebsocket *wsi = conn->wsi;
    usleep(80000);
    force_exit = 1;
    libwebsocket_cancel_service(context);
    libwebsocket_context_destroy(context);
}


void signal_connect(struct libwebsocket_context *context, volatile int *exit)
{
    fprintf(stderr, "SIGNALING: waiting for connect...\n");
    int n = 0;
    while( n >= 0 && !(*exit)){
        n = libwebsocket_service(context, 20);
    }
    
    fprintf(stderr, "SIGNALING: close connect\n\n");
}
