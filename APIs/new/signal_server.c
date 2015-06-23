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

void delete_req(char *requested_peer_name)
{
    bson_t *doc = bson_new();
    bson_error_t error;
    BSON_APPEND_UTF8 (doc, "requested_peer_name", requested_peer_name);

    if (!mongoc_collection_remove (req_coll, MONGOC_DELETE_SINGLE_REMOVE, doc, NULL, &error)) 
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


int peer_name_isExist(mongoc_collection_t *coll, char *peer_name)
{
    mongoc_cursor_t *cursor = NULL;
    const bson_t *find_context = NULL ;
    bson_t *find_query = bson_new();
    BSON_APPEND_UTF8(find_query, "peer_name", peer_name);
    cursor = mongoc_collection_find (coll, MONGOC_QUERY_NONE, 0, 0, 0, find_query, NULL, NULL);
    if ( mongoc_cursor_next(cursor, &find_context) == 0)
        return 0;
    else
        return 1;
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


struct SDP_context *SDP_context_isExist(char *peer_name)
{
    mongoc_cursor_t *cursor = NULL;
    const bson_t *find_context = NULL ;
    bson_t *find_query = bson_new();
    bson_t *proj = bson_new();
    BSON_APPEND_UTF8(find_query, "peer_name", peer_name);
    cursor = mongoc_collection_find (SDP_coll, MONGOC_QUERY_NONE, 0, 0, 0, find_query, NULL, NULL);

    if ( mongoc_cursor_next(cursor, &find_context) == 0)
        return NULL;
    else{
        bson_iter_t iter, value;
        bson_iter_init(&iter, find_context);
        bson_iter_find_descendant(&iter, "SDP", &value);
        uint32_t len = DATASIZE;
        const char *SDP = bson_iter_utf8(&value, &len);
        bson_iter_find_descendant(&iter, "candidate", &value);
        const char *candidate = bson_iter_utf8(&value, &len);
        if(strcmp(candidate,"") == 0 || strcmp(SDP, "") ==0 )
            return NULL;
        else{
            struct SDP_context *SDP_context_ins = (struct SDP_context *)calloc(1, sizeof(struct SDP_context));
            strcpy(SDP_context_ins->peer_name, peer_name);
            strcpy(SDP_context_ins->SDP, SDP);
            strcpy(SDP_context_ins->candidate, candidate);
            return SDP_context_ins;
        }
    }

}



void insert_candidate_in_queue(char *peer_name, char *candidate)
{
    // check peer_name is existed in SDP_coll,
    // if yes, update the candidate value
    // if no, insert 

    bson_error_t error;
    // check
    if (peer_name_isExist(SDP_coll, peer_name))
    {
        // update candidate value
        bson_t *update_context = NULL;
        bson_t *update_query = NULL;
        update_query = BCON_NEW ("peer_name", peer_name);
        update_context = BCON_NEW ("$set", "{",
                "candidate", BCON_UTF8 (candidate),
                "}");
        if (!mongoc_collection_update (SDP_coll, MONGOC_UPDATE_NONE, update_query, update_context, NULL, &error)) 
            printf ("%s\n", error.message);

    }
    else{
        // insert
        bson_oid_t oid;
        bson_t *insert_context = bson_new();
        bson_oid_init (&oid, NULL);
        BSON_APPEND_OID (insert_context, "_id", &oid);
        BSON_APPEND_UTF8 (insert_context, "peer_name", peer_name);
        BSON_APPEND_UTF8 (insert_context, "SDP", "");
        BSON_APPEND_UTF8 (insert_context, "candidate", candidate);

        if (!mongoc_collection_insert (SDP_coll, MONGOC_INSERT_NONE, insert_context, NULL, &error))
            printf ("%s\n", error.message);
    }

}

void insert_peer_in_queue(char *peer_name, char*SDP)
{
    // check peer_name is existed in SDP_coll,
    // if yes, update the SDP value
    // if no, insert 

    bson_error_t error;
    // check
    if (peer_name_isExist(SDP_coll, peer_name))
    {
        // update candidate value
        bson_t *update_context = NULL;
        bson_t *update_query = NULL;
        update_query = BCON_NEW ("peer_name", peer_name);
        update_context = BCON_NEW ("$set", "{",
                "SDP", BCON_UTF8 (SDP),
                "}");
        if (!mongoc_collection_update (SDP_coll, MONGOC_UPDATE_NONE, update_query, update_context, NULL, &error)) 
            printf ("%s\n", error.message);

    }
    else{
        // insert
        bson_oid_t oid;
        bson_t *insert_context = bson_new();
        bson_oid_init (&oid, NULL);
        BSON_APPEND_OID (insert_context, "_id", &oid);
        BSON_APPEND_UTF8 (insert_context, "peer_name", peer_name);
        BSON_APPEND_UTF8 (insert_context, "SDP", SDP);
        BSON_APPEND_UTF8 (insert_context, "candidate", "");

        if (!mongoc_collection_insert (SDP_coll, MONGOC_INSERT_NONE, insert_context, NULL, &error))
            printf ("%s\n", error.message);
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
            break;
        case LWS_CALLBACK_RECEIVE:
            recvd_session_data = (struct signal_session_data_t *)in;
            switch (recvd_session_data->type){
                case FILESERVER_REGISTER_t:
                    fprintf(stderr, "SIGNAL_SERVER: receive FILESERVER_REGISTER_t\n");
                    // insert the fileserver info in mongoDB
                    insert_peer("toppano://server1.tw", wsi);
                    
                    //send FILESERVER_REGISTER_OK_t back to file server 
                    sent_session_data = (struct signal_session_data_t *)calloc(1, sizeof(struct signal_session_data_t));
                    sent_session_data->type = FILESERVER_REGISTER_OK_t;
                    // the file server dns is assigned by signaling server
                    strcpy(sent_session_data->fileserver_dns, "toppano://server1.tw");
                    result = libwebsocket_write(wsi, (char *)sent_session_data, sizeof(struct signal_session_data_t), LWS_WRITE_TEXT);
                    free(sent_session_data);
                    break;
     
                case CLIENT_SDP_t:
                    fprintf(stderr, "SIGNAL_SERVER: receive CLIENT_SDP_t\n");
                    // whenever receiving client sdp, send to a fileserver 
                    // which the fileserver peer name is assign by signaling server
                    // and record the client wsi and assign a client dns;
                    insert_peer("client1", wsi);
                    
                    fileserver_wsi = get_wsi("toppano://server1.tw");
                    // transfer to the file server
                    sent_session_data = (struct signal_session_data_t *)calloc(1, sizeof(struct signal_session_data_t));
                    sent_session_data->type = CLIENT_SDP_t;
                    strcpy(sent_session_data->SDP, recvd_session_data->SDP);
                    strcpy(sent_session_data->candidates, recvd_session_data->candidates);
                    strcpy(sent_session_data->fileserver_dns, "toppano://server1.tw");
                    strcpy(sent_session_data->client_dns, "client1");
                    result = libwebsocket_write(fileserver_wsi, (char *)sent_session_data, sizeof(struct signal_session_data_t), LWS_WRITE_TEXT);
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
