#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <libwebsockets.h>
#include <string.h>
#include <bson.h>
#include <mongoc.h>
#include "signaling.h"
#define DEBUG_SIGNAL

static struct libwebsocket_context *context;
static volatile int force_exit = 0;

static mongoc_client_t *client;

static mongoc_collection_t *peer_coll;

static mongoc_collection_t *SDP_coll;
static mongoc_collection_t *req_coll;

void delete_peer(struct libwebsocket *peer_wsi)
{
    bson_t *doc = bson_new();
    bson_error_t error;
    intptr_t wsi_ptr = (intptr_t)peer_wsi;
    BSON_APPEND_INT32 (doc, "wsi", wsi_ptr);

    if (!mongoc_collection_remove (peer_coll, MONGOC_DELETE_SINGLE_REMOVE, doc, NULL, &error)) 
    {
        printf ("%s\n", error.message);
    }
}


void insert_peer(char *peer_name, struct libwebsocket *peer_wsi)
{
    bson_t *doc = bson_new();
    bson_oid_t oid;
    bson_error_t error;

    bson_oid_init (&oid, NULL);
    BSON_APPEND_OID (doc, "_id", &oid);
    BSON_APPEND_UTF8 (doc, "peer_name", peer_name);
    intptr_t wsi_ptr = (intptr_t)peer_wsi;
    BSON_APPEND_INT32 (doc, "wsi", wsi_ptr);

    if (!mongoc_collection_insert (peer_coll, MONGOC_INSERT_NONE, doc, NULL, &error)) 
    {
        printf ("%s\n", error.message);
    }

}




struct libwebsocket* get_wsi(char *peer_name)
{
    mongoc_cursor_t *cursor = NULL;
    const bson_t *find_context = NULL ;
    bson_t *find_query = bson_new();
    bson_t *proj = bson_new();
    BSON_APPEND_UTF8(find_query, "peer_name", peer_name);
    BSON_APPEND_BOOL(proj, "wsi", true);
    cursor = mongoc_collection_find (peer_coll, MONGOC_QUERY_NONE, 0, 0, 0, find_query, proj, NULL);

    if ( mongoc_cursor_next(cursor, &find_context) == 0){
        fprintf(stderr, "peer name %s does not exist in mongoDB\n", peer_name);
        return NULL;
    }
    else{
        bson_iter_t iter, value;
        bson_iter_init(&iter, find_context);
        bson_iter_find_descendant(&iter, "wsi", &value);
        BSON_ITER_HOLDS_INT32(&value);
        intptr_t wsi_ptr = (intptr_t) bson_iter_int32(&value);
        struct libwebsocket *wsi = (struct libwebsocket *)wsi_ptr;
        return wsi;
    }
}

static int callback_http(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len)
{
    switch (reason) {
        case LWS_CALLBACK_HTTP:
            break;
        default:
            break;

    }
    return 0;

}

static int callback_SDP(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len)
{
    struct signal_session_data_t *sent_session_data = NULL;
    struct signal_session_data_t *recvd_session_data = NULL;

    struct libwebsocket *fileserver_wsi = NULL;
    struct libwebsocket *client_wsi = NULL;
    int result;

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            fprintf(stderr, "SIGNAL_SERVER: LWS_CALLBACK_ESTABLISHED\n");
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            fprintf(stderr, "SIGNAL_SERVER: LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n");
            break;
        case LWS_CALLBACK_CLOSED:
            fprintf(stderr, "SIGNAL_SERVER: LWS_CALLBACK_CLOSED\n");
            // deregister the peer name and peer wsi in mongoDB
            delete_peer(wsi);
            break;
        case LWS_CALLBACK_RECEIVE:
            recvd_session_data = (struct signal_session_data_t *)in;
            switch (recvd_session_data->type){
                case FS_REGISTER_t:
                    fprintf(stderr, "SIGNAL_SERVER: receive FS_REGISTER_t\n");
                    // insert the fileserver info in mongoDB
                    insert_peer("toppano://server1.tw", wsi);
                    
                    //send FS_REGISTER_OK_t back to file server 
                    sent_session_data = (struct signal_session_data_t *)calloc(1, sizeof(struct signal_session_data_t));
                    sent_session_data->type = FS_REGISTER_OK_t;
                    // the file server dns is assigned by signaling server
                    strcpy(sent_session_data->fileserver_dns, "toppano://server1.tw");
                    result = libwebsocket_write(wsi, (char *)sent_session_data, sizeof(struct signal_session_data_t), LWS_WRITE_TEXT);
                    free(sent_session_data);
                    break;
     
                case SY_SDP:
                    fprintf(stderr, "SIGNAL_SERVER: receive CLIENT_SDP_t\n");
                    // whenever receiving client sdp, send to a fileserver 
                    // which the fileserver peer name is assign by signaling server
                    // and record the client wsi and assign a client dns;
                    insert_peer("client1", wsi);
                    
                    fileserver_wsi = get_wsi("toppano://server1.tw");
                    // transfer to the file server
                    sent_session_data = (struct signal_session_data_t *)calloc(1, sizeof(struct signal_session_data_t));
                    sent_session_data->type = SY_SDP;
                    strcpy(sent_session_data->SDP, recvd_session_data->SDP);
                    strcpy(sent_session_data->candidates, recvd_session_data->candidates);
                    strcpy(sent_session_data->fileserver_dns, "toppano://server1.tw");
                    strcpy(sent_session_data->client_dns, "client1");
                    result = libwebsocket_write(fileserver_wsi, (char *)sent_session_data, sizeof(struct signal_session_data_t), LWS_WRITE_TEXT);
                    free(sent_session_data);
                    break;

                 case FS_SDP:
                    fprintf(stderr, "SIGNAL_SERVER: receive FS_SDP\n");
                    
                    client_wsi = get_wsi(recvd_session_data->client_dns);
                    // transfer to the file server
                    sent_session_data = (struct signal_session_data_t *)calloc(1, sizeof(struct signal_session_data_t));
                    sent_session_data->type = FS_SDP;
                    strcpy(sent_session_data->SDP, recvd_session_data->SDP);
                    strcpy(sent_session_data->candidates, recvd_session_data->candidates);
                    strcpy(sent_session_data->fileserver_dns, recvd_session_data->fileserver_dns);
                    strcpy(sent_session_data->client_dns, recvd_session_data->client_dns);
                    result = libwebsocket_write(client_wsi, (char *)sent_session_data, sizeof(struct signal_session_data_t), LWS_WRITE_TEXT);
                    free(sent_session_data);
                    break;

                default:
                    break;
            }

        case LWS_CALLBACK_SERVER_WRITEABLE:
            break;
        default:
            break;
    }
    return 0;
}
static struct libwebsocket_protocols protocols[] = {
    {
        "http-only",
        callback_http,
        0,
        0,
    },
    {
        "fileserver-protocol",
        callback_SDP,
        sizeof(struct signal_session_data_t),
        10,
    },
    {
        "client-protocol",
        callback_SDP,
        sizeof(struct signal_session_data_t),
        10,
    },
    { NULL, NULL, 0, 0 }
};

void sighandler(int sig)
{
    force_exit = 1;
    libwebsocket_cancel_service(context);
}


int main()
{
    char cert_path[1024];
    char key_path[1024];
    int use_ssl = 0;
    int debug_level = 7;
    const char *iface = NULL;
    struct lws_context_creation_info info;
    int n;
    int opts = 0;
    int daemonize = 0;
    int syslog_options = LOG_PID | LOG_PERROR; 

    memset(&info, 0, sizeof(info));
    info.port = 7681;

    signal(SIGINT, sighandler);

    setlogmask(LOG_UPTO (LOG_DEBUG));
    openlog("lwsts", syslog_options, LOG_DAEMON);
    lws_set_log_level(debug_level, lwsl_emit_syslog);


    info.iface = iface;
    info.protocols = protocols;
    info.extensions = libwebsocket_get_internal_extensions();

    info.ssl_cert_filepath = NULL;
    info.ssl_private_key_filepath = NULL;

    info.gid = -1;
    info.uid = -1;
    info.options = opts;

    context = libwebsocket_create_context(&info);

    if (context == NULL) {
        lwsl_err("create context failed\n");
        return -1;
    }
    n = 0;


    /* initial the connection to mongodb */
    mongoc_init ();
    client = mongoc_client_new ("mongodb://localhost:27017/");
    peer_coll = mongoc_client_get_collection (client, "signal", "peer");

    while( n>=0 && !force_exit){
        n = libwebsocket_service(context, 50);
    }

    libwebsocket_context_destroy(context); 
    lwsl_notice("libwebsockets-test-server exited cleanly\n"); 

    closelog();

    return 0;
}
