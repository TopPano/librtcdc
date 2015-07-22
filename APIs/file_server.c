#include <stdio.h>
#include <rtcdc.h>
#include <string.h>

#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <openssl/md5.h>

#include <errno.h>
#include <jansson.h>
#include <sys/stat.h>
#include "signaling.h"

#define WAREHOUSE_PATH "/home/uniray/warehouse/"
#define LINE_SIZE 32


typedef struct rtcdc_peer_connection rtcdc_peer_connection;
typedef struct rtcdc_data_channel rtcdc_data_channel;

struct conn_info_t *signal_conn;
static volatile int fileserver_exit;

char *gen_md5(char *filename)
{
    FILE *pFile;
    pFile = fopen(filename, "r");
    int nread;
    unsigned char *hash = (unsigned char *)calloc(1, MD5_DIGEST_LENGTH);
    MD5_CTX ctx;
    MD5_Init(&ctx);
    char buf[LINE_SIZE];
    memset(buf, 0, LINE_SIZE);
    if(pFile)
    {
        while ((nread = fread(buf, 1, sizeof buf, pFile)) > 0)
        {    
            fwrite(buf, nread, 1, stdout); 
            memset(buf, 0, LINE_SIZE);
            MD5_Update(&ctx, (const void *)buf, nread);
        }

        if (ferror(pFile)) {
            /* deal with error */
        }
        fclose(pFile);
    }
    MD5_Final(hash, &ctx);

    char *result = (char *)calloc(1, MD5_DIGEST_LENGTH*2);
    memset(result, 0, MD5_DIGEST_LENGTH*2);
    int i;
    char tmp[4];
    for(i=0;i<MD5_DIGEST_LENGTH; i++)
    {
        sprintf(tmp,"%02x",hash[i]);
        strcat(result, tmp);
    }
    free(hash);
    return result;
}

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
    struct writedata_info_t *write_data = (struct writedata_info_t *)user;
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            {
                // printf("pty:%p\n", user);
                fprintf(stderr, "FILE_SERVER: LWS_CALLBACK_CLIENT_ESTABLISHED\n");
                /* register to metadata server */
                json_t *sent_session_JData = json_object();
                json_object_set_new(sent_session_JData, "metadata_type", json_integer(FS_REGISTER_t)); 
                char *sent_data_str = json_dumps(sent_session_JData, 0);

                write_data->target_wsi = wsi;
                write_data->type = FS_REGISTER_t;
                strcpy(write_data->data, sent_data_str);
                free(sent_data_str);
                json_decref(sent_session_JData);
                sent_session_JData = NULL;

                libwebsocket_callback_on_writable(context, wsi);
                break;
            }
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
                int lws_err;
                lws_err = libwebsocket_write(write_data->target_wsi, (void *) write_data->data, strlen(write_data->data), LWS_WRITE_TEXT);
                switch(write_data->type){
                    case FS_REGISTER_t:
                        {
#ifdef DEBUG_FS
                            if(lws_err<0)
                                fprintf(stderr, "FILE_SERVER: send FS_REGISTER_t fail\n");
                            else 
                                fprintf(stderr, "FILE_SERVER: send FS_REGISTER_t\n");
#endif
                            break;
                        }

                    case FS_INIT_OK:
                        {
#ifdef DEBUG_FS
                            if(lws_err<0)
                                fprintf(stderr, "FILE_SERVER: send FS_INIT_OK fail\n");
                            else 
                                fprintf(stderr, "FILE_SERVER: send FS_INIT_OK\n");
#endif
                            break;
                        }

                    case FS_STATUS_OK:
                        {
#ifdef DEBUG_FS
                            if(lws_err<0)
                                fprintf(stderr, "FILE_SERVER: send FS_STATUS_OK fail\n");
                            else 
                                fprintf(stderr, "FILE_SERVER: send FS_STATUS_OK\n");
#endif
                            break;
                        }

                    default:
                        break;
                }
                break;
            }
        case LWS_CALLBACK_CLIENT_RECEIVE:
            {
                int metadata_type;
                char *recvd_data_str = (char *)in;
                char *sent_data_str = NULL;
                json_t *recvd_session_JData = NULL;
                json_error_t *err = NULL;
                recvd_session_JData = json_loads((const char *)recvd_data_str, JSON_DECODE_ANY, err);
                json_unpack(recvd_session_JData, "{s:i}", "metadata_type", &metadata_type);
                switch(metadata_type){
                    case FS_REGISTER_OK_t:
                        {
#ifdef DEBUG_FS
                            fprintf(stderr, "FILE_SERVER: receive FS_REGISTER_OK_t\n");
#endif
                            break;
                        }
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
                            uint8_t mkdir_err = mkdir((const char *)repo_path, S_IRWXU);
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
                            json_t *sent_session_JData = json_object();
                            json_object_set_new(sent_session_JData, "metadata_type", json_integer(type)); 
                            json_object_set_new(sent_session_JData, "session_id", json_string(session_id)); 
                            sent_data_str = json_dumps(sent_session_JData, 0);
                            write_data->target_wsi = wsi;
                            write_data->type = type;
                            strcpy(write_data->data, sent_data_str);
                            
                            libwebsocket_callback_on_writable(context, wsi);
                            
                            free(repo_path);
                            free(repo_name);
                            free(session_id);
                            free(sent_data_str);
                            json_decref(sent_session_JData);
                            free(recvd_session_JData);
                            break;
                        }
                    case SY_STATUS:
                        {
#ifdef DEBUG_FS
                            fprintf(stderr, "FILE_SERVER: receive SY_STATUS: %s\n", recvd_data_str);
#endif
                            char *repo_name, *session_id;
                            char *repo_path = (char *)calloc(1, 128*sizeof(char));
                            json_unpack(recvd_session_JData, "{s:s, s:s}", "repo_name", &repo_name, "session_id", &session_id);
                            strcpy(repo_path, WAREHOUSE_PATH);
                            strcat(repo_path, repo_name);
 
                            /* calculate the file checksum under the repo_name */ 
                            /* and insert into a json object */
                            json_t *sent_session_JData = json_object();
                            json_t *json_arr = json_array();
                            json_object_set_new(sent_session_JData, "metadata_type", json_integer(FS_STATUS_OK));
                            json_object_set_new(sent_session_JData, "session_id", json_string(session_id));
                            json_object_set_new(sent_session_JData, "client_checksum", json_arr);
                            DIR *dp;
                            struct dirent *ep;
                            dp = opendir(repo_path);

                            if (dp != NULL)
                            {
                                while (ep = readdir (dp))
                                {    
                                    if(strcmp((const char*)ep->d_name, ".") && strcmp((const char*)ep->d_name, "..") != 0)
                                    {
                                        json_t *file_json = json_object();
                                        printf("%s\n", (ep->d_name));
                                        /* calculate the file checksum and put it in json object */
                                        char *file_md5 = gen_md5(ep->d_name);
                                        printf("%s\n", file_md5);
                                        json_object_set_new(file_json, "filename", json_string(ep->d_name));
                                        json_object_set_new(file_json, "MD5", json_string(file_md5));
                                        json_array_append(json_arr, file_json);
                                        json_decref(file_json);
                                        free(file_md5);
                                    }
                                }
                                (void) closedir (dp);
                            }
                            else
                                perror ("Couldn't open the directory"); 

                            /* send the FS_STATUS, json object and session_id back to the metadata server */
                            char *sent_data_str = json_dumps(sent_session_JData, 0);
                            write_data->target_wsi = wsi;
                            write_data->type = FS_STATUS_OK;
                            strcpy(write_data->data, sent_data_str);
                            libwebsocket_callback_on_writable(context, wsi);
                            
                            break;
                        }
                    default:
                        break;
                }
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
