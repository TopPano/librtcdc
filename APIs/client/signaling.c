#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include "signaling.h"

static struct libwebsocket_context *context;
static volatile int force_exit = 0;
static struct session_data__SDP* signal_session_data;
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


static int callback_SDP(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len)
{
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            recvd_SDP = NULL;
            fprintf(stderr, "SDP: LWS_CALLBACK_CLIENT_ESTABLISHED\n");
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            fprintf(stderr, "LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n");
            break;

        case LWS_CALLBACK_CLOSED:
            fprintf(stderr, "LWS_CALLBACK_CLOSED\n");
            force_exit = 1;
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            // 1. get local SDP and then send to the server
            libwebsocket_write(wsi, (char *) signal_session_data, sizeof(struct session_data__SDP), LWS_WRITE_TEXT);
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            if(((struct SDP_context *)in)->state == RECV_SDP_CONTEXT)
            {
                pthread_mutex_lock(&mutex);
                recvd_SDP = (struct SDP_context *)in;
                pthread_mutex_unlock(&mutex);
            }
            break;

        default:
            break;
    }
    return 0;
}


static struct libwebsocket_protocols protocols[] = {
    {
        "SDP-protocol",
        callback_SDP,
        sizeof(struct session_data__SDP),
        10,
    },
    { NULL, NULL, 0, 0 }
};



struct SDP_context* signal_req(struct signal_conn_info* SDP_conn, char *peer_name)
{
    struct SDP_context *SDP = NULL;
    struct session_data__SDP *session_data = (struct session_data__SDP *)calloc(1, sizeof(struct session_data__SDP));
    session_data->state = REQ_SDP_CONTEXT;
    strcpy(session_data->peer_name, peer_name);
    memset(session_data->data, 0, DATASIZE);
    signal_send(SDP_conn, session_data);

    //poll adn wait
    while(SDP == NULL){
        pthread_mutex_lock(&mutex);
        if(recvd_SDP != NULL && recvd_SDP->state == RECV_SDP_CONTEXT)
        {
            SDP = recvd_SDP; 
        }
        pthread_mutex_unlock(&mutex);
        usleep(500000);

    }
    return SDP;
}

void signal_send(struct signal_conn_info* SDP_conn, struct session_data__SDP *session_data)
{
    struct libwebsocket *wsi = SDP_conn->wsi;
    struct libwebsocket_context *context = SDP_conn->context;
    signal_session_data = session_data;

    /*FIX ME*/
    usleep(25000);
    libwebsocket_callback_on_writable(context, wsi);
    usleep(25000);
}


void signal_send_SDP(struct signal_conn_info *SDP_conn, char *peer_name, char *SDP)
{
    struct session_data__SDP *session_data = (struct session_data__SDP *)calloc(1, sizeof(struct session_data__SDP));
    session_data->state = SEND_LOCAL_SDP;
    strcpy(session_data->peer_name, peer_name);
    strcpy(session_data->data, SDP);
    signal_send(SDP_conn, session_data);
}


void signal_send_candidate(struct signal_conn_info *SDP_conn, char *peer_name, char *candidate)
{
    struct session_data__SDP *session_data = (struct session_data__SDP *)calloc(1, sizeof(struct session_data__SDP));
    session_data->state = SEND_LOCAL_CANDIDATE;
    strcpy(session_data->peer_name, peer_name);
    strcpy(session_data->data, candidate);
    signal_send(SDP_conn, session_data);
}


struct signal_conn* signal_initial(const char *address, int port)
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


    struct signal_conn_info* SDP_conn = (struct signal_conn_info *)calloc(1, sizeof(struct signal_conn_info)); 
    context = libwebsocket_create_context(&info);
    if (context == NULL) {
        lwsl_err("libwebsocket_create_context failed\n");
        goto bail;
    }
    SDP_conn->context = context;

    wsi = libwebsocket_client_connect(context, address, port, use_ssl, "/", 
            address, address, "SDP-protocol", ietf_version);
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


void signal_close(struct signal_conn_info* conn_info)
{
    fprintf(stderr, "close the connect\n");
    struct libwebsocket_context *context = conn_info->context;
    struct libwebsocket *wsi = conn_info->wsi;
    usleep(80000);
    force_exit = 1;
    libwebsocket_cancel_service(context);
    libwebsocket_context_destroy(context);
}


void* signal_connect(void *pthread_data)
{
    /*FIX: need allocate a thread to run the while loop*/ 
    fprintf(stderr, "waiting for connect...\n");
    struct libwebsocket_context *context = (struct libwebsocket_context *)pthread_data;
    int n = 0;
    while( n >= 0 && !force_exit){
        n = libwebsocket_service(context, 20);
    }

}
