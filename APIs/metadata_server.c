#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <libwebsockets.h>
#include <string.h>
#include <jansson.h>
#include <bson.h>
#include <mongoc.h>
#include "signaling.h"

static struct libwebsocket_context *context;
static volatile int force_exit = 0;

static mongoc_client_t *client;

static mongoc_collection_t *fileserver_coll;
static mongoc_collection_t *session_coll;
static mongoc_collection_t *filesys_coll;

void delete_fileserver(struct libwebsocket *fileserver_wsi)
{
    bson_t *doc = bson_new();
    bson_error_t error;
    intptr_t wsi_ptr = (intptr_t)fileserver_wsi;
    BSON_APPEND_INT32 (doc, "wsi", wsi_ptr);

    if (!mongoc_collection_remove (fileserver_coll, MONGOC_DELETE_SINGLE_REMOVE, doc, NULL, &error)) 
    {
        printf ("%s\n", error.message);
    }
}


uint8_t insert_fileserver(char *fileserver_name, struct libwebsocket *fileserver_wsi)
{
    bson_t *doc = bson_new();
    bson_oid_t oid;
    bson_error_t error;

    bson_oid_init (&oid, NULL);
    BSON_APPEND_OID (doc, "_id", &oid);
    BSON_APPEND_UTF8 (doc, "fileserver_name", fileserver_name);
    intptr_t wsi_ptr = (intptr_t)fileserver_wsi;
    BSON_APPEND_INT32 (doc, "wsi", wsi_ptr);

    if (!mongoc_collection_insert (fileserver_coll, MONGOC_INSERT_NONE, doc, NULL, &error)) {
#ifdef DEBUG_META
        fprintf(stderr, "METADATA_SERVER: insert_fileserver(): %s\n", error.message);
#endif
        return 0;
    }
    return 1;
}

char *insert_session(struct libwebsocket *fileserver_wsi, struct libwebsocket *client_wsi, char *repo_name, METADATAtype state)
{
    char *session_id = (char *)calloc(1, 25);
    bson_oid_t oid;
    bson_oid_init (&oid, NULL);
    bson_oid_to_string(&oid, session_id);

    bson_t *doc = bson_new();
    bson_error_t error;

    BSON_APPEND_UTF8 (doc, "session_id", session_id);
    intptr_t client_wsi_ptr = (intptr_t)client_wsi;
    intptr_t fileserver_wsi_ptr = (intptr_t)fileserver_wsi;
    BSON_APPEND_INT32 (doc, "client_wsi", client_wsi_ptr);
    BSON_APPEND_INT32 (doc, "fileserver_wsi", fileserver_wsi_ptr);
    BSON_APPEND_UTF8 (doc, "repo_name", repo_name);
    BSON_APPEND_INT32 (doc, "state", state);

    if (!mongoc_collection_insert (session_coll, MONGOC_INSERT_NONE, doc, NULL, &error)){
#ifdef DEBUG_META
        fprintf (stderr, "METADATA_SERVER: insert_session(): %s\n", error.message);
#endif
        return NULL;
    }
    return session_id;
}


uint8_t insert_filesys(struct URI_info_t *repo_URI)
{
    bson_t *doc = bson_new();
    bson_error_t error;

    BSON_APPEND_UTF8 (doc, "URI_code", repo_URI->URI_code);
    intptr_t fileserver_wsi_ptr = (intptr_t)repo_URI->fileserver_wsi;
    BSON_APPEND_INT32 (doc, "fileserver_wsi", fileserver_wsi_ptr);
    BSON_APPEND_UTF8 (doc, "repo_name", repo_URI->repo_name);

    if (!mongoc_collection_insert (filesys_coll, MONGOC_INSERT_NONE, doc, NULL, &error)){
#ifdef DEBUG_META
        fprintf (stderr, "METADATA_SERVER: insert_filesys(): %s\n", error.message);
#endif
        return 0;
    }
    return 1;
}


uint8_t update_session(char *session_id, char *field, void *value)
{
    bson_t *update = NULL;
    bson_t *query = NULL;
    bson_error_t error;
    query = BCON_NEW ("session_id", BCON_UTF8(session_id));
    
    if(strcmp(field, "client_wsi") == 0){
        update = BCON_NEW ("$set", "{",
                "client_wsi", BCON_INT32 (*(int *)value),
                "}");
    }
    else if (strcmp(field, "state") == 0){
        update = BCON_NEW ("$set", "{",
                "state", BCON_INT32 (*(int *)value),
                "}");
    }
    else{
#ifdef DEBUG_META
        fprintf(stderr, "METADATA_SERVER: update_session(): the field not exist or the field cannot be changed(e.g repo_name)\n");
#endif
        return 0;
    }

    if (!mongoc_collection_update (session_coll, MONGOC_UPDATE_NONE, query, update, NULL, &error)) {
#if DEBUG_META
        fprintf (stderr, "METADATA_SERVER: update_session(): %s\n", error.message);
#endif
        return 0;
    }
    return 1;
}

struct session_info_t *get_session(char *session_id)
{
    mongoc_cursor_t *cursor = NULL;
    const bson_t *find_context = NULL ;
    bson_t *find_query = bson_new();
    bson_t *proj = bson_new();
    BSON_APPEND_UTF8(find_query, "session_id", session_id);
    BSON_APPEND_BOOL(proj, "fileserver_wsi", true);
    BSON_APPEND_BOOL(proj, "client_wsi", true);
    BSON_APPEND_BOOL(proj, "repo_name", true);
    cursor = mongoc_collection_find (session_coll, MONGOC_QUERY_NONE, 0, 0, 0, find_query, proj, NULL);

    struct session_info_t *session = NULL;
    if ( mongoc_cursor_next(cursor, &find_context) == 0){
#ifdef DEBUG_META
        fprintf(stderr, "METADATA_SERVER: session_id %s does not exist in db.session\n", session_id);
#endif
        return NULL;
    }
    else{
        session = (struct session_info_t *)calloc(1, sizeof(struct session_info_t));
        bson_iter_t iter, value;
        bson_iter_init(&iter, find_context);
        bson_iter_find_descendant(&iter, "fileserver_wsi", &value);
        BSON_ITER_HOLDS_INT32(&value);
        intptr_t fileserver_wsi_ptr = (intptr_t) bson_iter_int32(&value);
        session->fileserver_wsi = (struct libwebsocket *)fileserver_wsi_ptr;
       
        bson_iter_init(&iter, find_context);
        bson_iter_find_descendant(&iter, "client_wsi", &value);
        BSON_ITER_HOLDS_INT32(&value);
        intptr_t client_wsi_ptr = (intptr_t) bson_iter_int32(&value);
        session->client_wsi = (struct libwebsocket *)client_wsi_ptr;

        bson_iter_init(&iter, find_context);
        bson_iter_find_descendant(&iter, "repo_name", &value);
        BSON_ITER_HOLDS_UTF8(&value);
        uint32_t length;
        strcpy(session->repo_name, (char *)bson_iter_utf8(&value, &length));
        
        strcpy(session->session_id, session_id);
    }
    return session;
}

struct URI_info_t *get_URI(char *URI_code)
{
    mongoc_cursor_t *cursor = NULL;
    const bson_t *find_context = NULL ;
    bson_t *find_query = bson_new();
    bson_t *proj = bson_new();
    BSON_APPEND_UTF8(find_query, "URI_code", URI_code);
    BSON_APPEND_BOOL(proj, "fileserver_wsi", true);
    BSON_APPEND_BOOL(proj, "repo_name", true);
    cursor = mongoc_collection_find (filesys_coll, MONGOC_QUERY_NONE, 0, 0, 0, find_query, proj, NULL);

    struct URI_info_t *URI = NULL;
    if ( mongoc_cursor_next(cursor, &find_context) == 0){
#ifdef DEBUG_META
        fprintf(stderr, "METADATA_SERVER: URI_code %s does not exist in db.filesys\n", URI_code);
#endif
        return NULL;
    }
    else{
        URI = (struct URI_info_t *)calloc(1, sizeof(struct URI_info_t));
        bson_iter_t iter, value;
        bson_iter_init(&iter, find_context);
        bson_iter_find_descendant(&iter, "fileserver_wsi", &value);
        BSON_ITER_HOLDS_INT32(&value);
        intptr_t fileserver_wsi_ptr = (intptr_t) bson_iter_int32(&value);
        URI->fileserver_wsi = (struct libwebsocket *)fileserver_wsi_ptr;
       
        bson_iter_init(&iter, find_context);
        bson_iter_find_descendant(&iter, "repo_name", &value);
        BSON_ITER_HOLDS_UTF8(&value);
        uint32_t length;
        strcpy(URI->repo_name, (char *)bson_iter_utf8(&value, &length));
        
        strcpy(URI->URI_code, URI_code);
    }
    return URI;
}

struct libwebsocket* get_wsi(char *fileserver_name)
{
    mongoc_cursor_t *cursor = NULL;
    const bson_t *find_context = NULL ;
    bson_t *find_query = bson_new();
    bson_t *proj = bson_new();
    BSON_APPEND_UTF8(find_query, "fileserver_name", fileserver_name);
    BSON_APPEND_BOOL(proj, "wsi", true);
    cursor = mongoc_collection_find (fileserver_coll, MONGOC_QUERY_NONE, 0, 0, 0, find_query, proj, NULL);

    if ( mongoc_cursor_next(cursor, &find_context) == 0){
#ifdef DEBUG_META
        fprintf(stderr, "METADATA_SERVER: fileserver name %s does not exist in db.fileserver\n", fileserver_name);
#endif
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
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            fprintf(stderr, "METADATA_SERVER: LWS_CALLBACK_ESTABLISHED\n");
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            fprintf(stderr, "METADATA_SERVER: LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n");
            break;
        case LWS_CALLBACK_CLOSED:
            fprintf(stderr, "METADATA_SERVER: LWS_CALLBACK_CLOSED\n");
            // deregister the fileserver name and fileserver wsi in mongoDB
            delete_fileserver(wsi);
            break;
        case LWS_CALLBACK_RECEIVE:
            {
                int metadata_type ,lws_err;
                char *session_SData = (char *)in;
                json_error_t *err = NULL;
                json_t *sent_session_JData = NULL;
                json_t *recvd_session_JData = NULL;
                recvd_session_JData = json_loads((const char *)session_SData, JSON_DECODE_ANY, err);
                json_unpack(recvd_session_JData, "{s:i}", "metadata_type", &metadata_type);

                switch (metadata_type){
                    case FS_REGISTER_t:
                        {
                            fprintf(stderr, "METADATA_SERVER: receive FS_REGISTER_t\n");
                            // insert the fileserver info in mongoDB
                            
                            if(insert_fileserver("toppano://server1.tw", wsi) == 0){
                                 /* TODO: if insert failed, what to do? */
                            }
                            //send FILESERVER_REGISTER_OK_t back to file server 
                            // the file server dns is assigned by signaling server
                            sent_session_JData = json_object();
                            json_object_set_new(sent_session_JData, "metadata_type", json_integer(FS_REGISTER_OK_t));
                            json_object_set_new(sent_session_JData, "fileserver_dns", json_string("toppano://server1.tw"));
                            session_SData = json_dumps(sent_session_JData, 0);
                            lws_err = libwebsocket_write(wsi, (void *)session_SData, strlen(session_SData), LWS_WRITE_TEXT);
#ifdef DEBUG_META
                            if(lws_err<0)
                                fprintf(stderr, "METADATA_SERVER: send FS_REGISTER_OK_t fail\n");
                            else
                                fprintf(stderr, "METADATA_SERVER: send FS_REGISTER_OK_t\n");
#endif
                            free(recvd_session_JData);   
                            free(sent_session_JData);
                            free(session_SData);
                            break;
                        }
                    case SY_INIT:
                        {
                            fprintf(stderr, "METADATA_SERVER: receive SY_INIT\n");
                            /* Oauth */

                            /* Insert (client_wsi, fileserver_wsi, repo_name, state, session_id) into session table */
                            char *session_id, *repo_name;
                            /* TODO: exception when  "toppano://server1.tw" not exist */
                            struct libwebsocket *fileserver_wsi = get_wsi("toppano://server1.tw");
                            if(fileserver_wsi == NULL)
                            {
                                fprintf(stderr, "METADATA_SERVER: %s not exist\n", "toppano://server1.tw");
                                break;
                            }
                            struct libwebsocket *client_wsi = wsi;
                            json_unpack(recvd_session_JData, "{s:s}", "repo_name", &repo_name);
                            if((session_id = insert_session(fileserver_wsi, client_wsi, repo_name, SY_INIT)) == NULL){
                                 /* TODO: if insert failed, what to do? */
                            }
                            /* send SY_INIT & repo_name & session_id to the fileserver*/
                            sent_session_JData = json_object();
                            json_object_set_new(sent_session_JData, "metadata_type", json_integer(SY_INIT));
                            json_object_set_new(sent_session_JData, "repo_name", json_string(repo_name));
                            json_object_set_new(sent_session_JData, "session_id", json_string(session_id));
                            session_SData = json_dumps(sent_session_JData, 0);
                            lws_err = libwebsocket_write(fileserver_wsi, (void *)session_SData, strlen(session_SData), LWS_WRITE_TEXT);
#ifdef DEBUG_META
                            if(lws_err<0)
                                fprintf(stderr, "METADATA_SERVER: send SY_INIT to fileserver fail\n");
                            else
                                fprintf(stderr, "METADATA_SERVER: send SY_INIT to fileserver\n");
#endif
                            free(recvd_session_JData);   
                            free(sent_session_JData);
                            free(session_SData);
                            break;
                        }
                    case FS_INIT_OK:
                        {                    
                            fprintf(stderr, "METADATA_SERVER: receive FS_INIT_OK\n");
                            char *session_id; 
                            json_unpack(recvd_session_JData, "{s:s}", "session_id", &session_id);
                            /* Gen a URI_code and insert into Filesys tree (URI_code, repo_name, fileserver_wsi) */
                            char URI_code[NAMESIZE];
                            bson_oid_t oid;
                            bson_oid_init (&oid, NULL);
                            bson_oid_to_string(&oid, URI_code);

                            /* lookup fileserver_wsi and repo_name */
                            struct session_info_t *session = get_session(session_id);
                            struct URI_info_t *repo_URI = (struct URI_info_t *)calloc(1, sizeof(struct URI_info_t));
                            strcpy(repo_URI->URI_code, URI_code);
                            strcpy(repo_URI->repo_name, session->repo_name);
                            repo_URI->fileserver_wsi = session->fileserver_wsi;

                            if(insert_filesys(repo_URI) == 0){
                                /* TODO: if insert failed, what to do? */
                            }
                            /* update the session state */
                            METADATAtype state = SY_INIT_OK;
                            if(update_session(session_id, "state", (void*)&state) == 0){
                                /* TODO: if update failed, what to do? */
                            }

                            /* send SY_INIT_OK, URI_code, session_id to client according to client_wsi in session table */
                            sent_session_JData = json_object();
                            json_object_set_new(sent_session_JData, "metadata_type", json_integer(SY_INIT_OK));
                            json_object_set_new(sent_session_JData, "URI_code", json_string(URI_code));
                            json_object_set_new(sent_session_JData, "session_id", json_string(session_id));
                            session_SData = json_dumps(sent_session_JData, 0);
                            lws_err = libwebsocket_write(session->client_wsi, (void *)session_SData, strlen(session_SData), LWS_WRITE_TEXT);
#ifdef DEBUG_META
                            if(lws_err<0)
                                fprintf(stderr, "METADATA_SERVER: send SY_INIT_OK to client fail\n");
                            else
                                fprintf(stderr, "METADATA_SERVER: send SY_INIT_OK to client\n");
#endif
                            free(recvd_session_JData);   
                            free(sent_session_JData);
                            free(session_SData);
                            break;
                        }
                
                    case SY_CONNECT:
                        {
                            fprintf(stderr, "METADATA_SERVER: receive SY_CONNECT\n");
                            /* TODO: Oauth */
                            
                            char *session_id, *URI_code, *repo_name;
                            struct libwebsocket *fileserver_wsi, *client_wsi;
                            /* lookup the URI_code existed, and get URI_info_t */

                            json_unpack(recvd_session_JData, "{s:s}", "URI_code", &URI_code);
                            struct URI_info_t *URI;
                            /* if the URI_code not exist, send SY_REPO_NOT_EXIST and break */
                            if((URI = get_URI(URI_code)) == NULL)
                            {
                                fprintf(stderr, "METADATA_SERVER: URI %s not exist\n", URI_code);
                                /* TODO: send SY_REPO_NOT_EXIST */
                                break;
                            }
                            
                            fileserver_wsi = URI->fileserver_wsi;
                            client_wsi = wsi;
                            /* Insert (client_wsi, fileserver_wsi, repo_name, state, session_id) into session table */
                            if((session_id = insert_session(fileserver_wsi, client_wsi, URI->repo_name, SY_CONNECT)) == NULL)
                            {
                                 /* TODO: if insert failed, what to do? */
                            }

                             
                            /* send SY_CONNECT_OK & session_id  back to client*/
                            sent_session_JData = json_object();
                            json_object_set_new(sent_session_JData, "metadata_type", json_integer(SY_CONNECT_OK));
                            json_object_set_new(sent_session_JData, "session_id", json_string(session_id));
                            session_SData = json_dumps(sent_session_JData, 0);
                            lws_err = libwebsocket_write(client_wsi, (void *)session_SData, strlen(session_SData), LWS_WRITE_TEXT);
#ifdef DEBUG_META
                            if(lws_err<0)
                                fprintf(stderr, "METADATA_SERVER: send SY_CONNECT_OK to client fail\n");
                            else
                                fprintf(stderr, "METADATA_SERVER: send SY_CONNECT_OK to client\n");
#endif
                            free(recvd_session_JData);   
                            free(sent_session_JData);
                            free(session_SData);
                            break;
                        }    
                    default:
                        break;
                }
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
        1024,
        10,
    },
    {
        "client-protocol",
        callback_SDP,
        1024,
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
    fileserver_coll = mongoc_client_get_collection (client, "metadata", "fileserver");
    session_coll = mongoc_client_get_collection (client, "metadata", "session");
    filesys_coll = mongoc_client_get_collection (client, "metadata", "filesys");
    while( n>=0 && !force_exit){
        n = libwebsocket_service(context, 50);
    }

    libwebsocket_context_destroy(context); 
    lwsl_notice("libwebsockets-test-server exited cleanly\n"); 

    closelog();

    return 0;
}
