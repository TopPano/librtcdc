#include <stdio.h>
#include <rtcdc.h>
#include <string.h>
#include <errno.h>
#include <jansson.h>
#include <sys/stat.h>
#include "signaling.h"

#define WAREHOUSE_PATH "/home/uniray/warehouse/"

typedef struct rtcdc_peer_connection rtcdc_peer_connection;
typedef struct rtcdc_data_channel rtcdc_data_channel;

struct conn_info_t *signal_conn;
static volatile int fileserver_exit;

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



static int callback_fileserver(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len)
{
    SESSIONstate *session_state = (SESSIONstate *)user;

    /* SData: string data*/
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
            {
                int metadata_type, lws_err;
                char *session_SData = NULL;
                json_t *sent_session_JData = NULL;
                if(*session_state == FILESERVER_INITIAL){
                    /* register to  signaling server */
                    sent_session_JData = json_object();
                    json_object_set_new(sent_session_JData, "metadata_type", json_integer(FS_REGISTER_t)); 
                    session_SData = json_dumps(sent_session_JData, 0);
                    lws_err = libwebsocket_write(wsi, (void *) session_SData, strlen(session_SData), LWS_WRITE_TEXT);
#ifdef DEBUG_FS
                    if(lws_err<0)
                        fprintf(stderr, "FILE_SERVER: send FS_REGISTER_t fail\n");
                    else 
                        fprintf(stderr, "FILE_SERVER: send FS_REGISTER_t\n");
#endif
                    json_decref(sent_session_JData);
                    sent_session_JData = NULL;
                }
                break;
            }
        case LWS_CALLBACK_CLIENT_RECEIVE:
            {
                int metadata_type, lws_err;
                char *session_SData = (char *)in;
                json_t *sent_session_JData = NULL;
                json_t *recvd_session_JData = NULL;
                json_error_t *err = NULL;

                recvd_session_JData = json_loads((const char *)session_SData, JSON_DECODE_ANY, err);
                json_unpack(recvd_session_JData, "{s:i}", "metadata_type", &metadata_type);

                if(*session_state == FILESERVER_INITIAL){ 
                    switch(metadata_type){
                        case FS_REGISTER_OK_t:
#ifdef DEBUG_FS
                            fprintf(stderr, "FILE_SERVER: receive FS_REGISTER_OK_t\n");
#endif
                            *session_state = FILESERVER_READY;
                            break;
                        default:
                            break;
                    }
                }
                else if(*session_state == FILESERVER_READY){
                    switch (metadata_type){
                        case SY_INIT:
                            {
#ifdef DEBUG_FS
                                fprintf(stderr, "FILE_SERVER: receive SY_INIT\n");
#endif
                                /* mkdir */
                                char *repo_name, *session_id;
                                char *repo_path = (char *)calloc(1, 128*sizeof(char));
                                json_unpack(recvd_session_JData, "{s:s, s:s}", "repo_name", &repo_name, "session_id", &session_id);
                                strcpy(repo_path, WAREHOUSE_PATH);
                                strcat(repo_path, repo_name);
                                int mkdir_err = mkdir((const char *)repo_path, S_IRWXU);
                                METADATAtype type;
                                if(mkdir_err == 0){
                                    fprintf(stderr, "FILE_SERVER: mkdir success\n");
                                    type = FS_INIT_OK;
                                }
                                else{
                                    fprintf(stderr, "FILE_SERVER: mkdir: %s\n", strerror(errno));
                                    /* TODO type should be FS_REPO_EXIST*/
                                    type = FS_INIT_OK;
                                }

                                /* send FS_INIT_OK back */
                                sent_session_JData = json_object();
                                json_object_set_new(sent_session_JData, "metadata_type", json_integer(type)); 
                                json_object_set_new(sent_session_JData, "session_id", json_string(session_id)); 
                                session_SData = json_dumps(sent_session_JData, 0);
                                lws_err = libwebsocket_write(wsi, (void *) session_SData, strlen(session_SData), LWS_WRITE_TEXT);
#ifdef DEBUG_FS
                                if(lws_err<0)
                                    fprintf(stderr, "FILE_SERVER: send FS_INIT_OK fail\n");
                                else 
                                    fprintf(stderr, "FILE_SERVER: send FS_INIT_OK\n");
#endif
                                json_decref(sent_session_JData);
                                free(recvd_session_JData);
                            }
                        case SY_STATUS:
                            /* calculate the file checksum under the repo_name */ 
                            /* and insert into a json object */
                            /* send the FS_STATUS, json object and session_id back to the metadata server */
                            
                            break;
                        default:
                            break;
                    }
                } // end if
                break;
            }
        default:
            break;
    }
    return 0;
}


static struct libwebsocket_protocols fileserver_protocols[] = {
    {
        "fileserver-protocol",
        callback_fileserver,
        1024,
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

