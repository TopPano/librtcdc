#include <stdio.h>
#include <rtcdc.h>
#include <string.h>
#include <uv.h>

#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <openssl/md5.h>

#include <errno.h>
#include <jansson.h>
#include <sys/stat.h>
#include "../lwst.h"
#include "fs_rtcdc.h"


#define WAREHOUSE_PATH "/home/uniray/warehouse/"
#define LINE_SIZE 32


typedef struct rtcdc_peer_connection rtcdc_peer_connection;
typedef struct rtcdc_data_channel rtcdc_data_channel;

struct lwst_conn_t *signal_conn;
static volatile int fileserver_exit;
//static uv_loop_t *main_loop;

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
            memset(buf, 0, LINE_SIZE);
            MD5_Update(&ctx, (const void *)buf, nread);
        }

        if (ferror(pFile)) {
            /* deal with error */
        }
        fclose(pFile);
    }
    MD5_Final(hash, &ctx);
    char *result = (char *)calloc(1, (MD5_DIGEST_LENGTH*2+1));
    memset(result, 0, (MD5_DIGEST_LENGTH*2+1));
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


static int callback_fileserver(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len)
{
    struct lwst_writedata_t **write_data_ptr = (struct lwst_writedata_t **)user;
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            {
                fprintf(stderr, "FILE_SERVER: LWS_CALLBACK_CLIENT_ESTABLISHED\n");
                /* initial write data */
                struct lwst_writedata_t *write_data = (struct lwst_writedata_t *)calloc(1, sizeof(struct lwst_writedata_t));
                *write_data_ptr = write_data;

                /* register to metadata server */
                json_t *sent_session_JData = json_object();
                json_object_set_new(sent_session_JData, "metadata_type", json_integer(FS_REGISTER_t)); 
                char *sent_data_str = json_dumps(sent_session_JData, 0);

                /* TODO: should allocate memory for write_data pointer? */
                write_data->target_wsi = wsi;
                write_data->type = FS_REGISTER_t;
                strcpy(write_data->data, sent_data_str);
                
                free(sent_data_str);
                free(sent_session_JData);

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
                size_t data_len;
                struct lwst_writedata_t *write_data = *write_data_ptr;
                if((data_len = strlen(write_data->data))>0)
                {
                    
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
                        case FS_UPLOAD_READY:
                            {
#ifdef DEBUG_FS
                                if(lws_err<0)
                                    fprintf(stderr, "FILE_SERVER: send FS_UPLOAD_READY fail\n");
                                else 
                                    fprintf(stderr, "FILE_SERVER: send FS_UPLOAD_READY\n");
#endif
                                break;
                            }
                        default:
                            break;
                    }                
                    memset(write_data->data, 0, 100);
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
                            
                            struct lwst_writedata_t *write_data = *write_data_ptr;
                            write_data->target_wsi = wsi;
                            write_data->type = type;
                            strcpy(write_data->data, sent_data_str);

                            libwebsocket_callback_on_writable(context, wsi);

                            free(repo_path);
                            free(repo_name);
                            free(session_id);
                            free(sent_data_str);
                            free(sent_session_JData);
                            free(recvd_session_JData);
                            break;
                        }
                    case SY_STATUS:
                        {
#ifdef DEBUG_FS
                            fprintf(stderr, "FILE_SERVER: receive SY_STATUS\n");
#endif
                            char *repo_name, *session_id;
                            json_unpack(recvd_session_JData, "{s:s, s:s}", "repo_name", &repo_name, "session_id", &session_id);
                            char *repo_path = (char *)calloc(1, 128*sizeof(char));
                            strcpy(repo_path, WAREHOUSE_PATH);
                            strcat(repo_path, repo_name);

                            /* calculate the file checksum under the repo_name */ 
                            /* and insert into a json object */
                            json_t *sent_session_JData = json_object();
                            json_t *json_arr = json_array();
                            json_object_set_new(sent_session_JData, "metadata_type", json_integer(FS_STATUS_OK));
                            json_object_set_new(sent_session_JData, "session_id", json_string(session_id));
                            json_object_set_new(sent_session_JData, "files", json_arr);
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
                                        //printf("%s\n", (ep->d_name));
                                        /* calculate the file checksum and put it in json object */
                                        char file_path[128];
                                        memset(file_path, 0, 128);
                                        strcpy(file_path, repo_path);
                                        strcat(file_path, "/");
                                        strcat(file_path, ep->d_name);
                                        char *file_md5 = gen_md5(file_path);
                                        //printf("%s\n", file_md5);
                                        json_object_set_new(file_json, "filename", json_string(ep->d_name));
                                        json_object_set_new(file_json, "fs_checksum", json_string(file_md5));
                                        json_array_append(json_arr, file_json);
                                        /* TODO file_josn needs to be free? */
                                        //json_decref(file_json);
                                        free(file_md5);
                                    }
                                }
                                (void) closedir (dp);
                            }
                            else
                                perror ("Couldn't open the directory"); 

                            /* send the FS_STATUS, json object and session_id back to the metadata server */
                            sent_data_str = json_dumps(sent_session_JData, 0);
                            
                            struct lwst_writedata_t *write_data = *write_data_ptr;
                            write_data->target_wsi = wsi;
                            write_data->type = FS_STATUS_OK;
                            strcpy(write_data->data, sent_data_str);
                            
                            libwebsocket_callback_on_writable(context, wsi);

                            break;
                        }
                    case SY_UPLOAD:
                        {
#ifdef DEBUG_FS
                            fprintf(stderr, "FILE_SERVER: receive SY_UPLOAD\n");
#endif
                            /* get repo_name, and strcat repo_path, get client_SDP and client_candidate */
                            char *client_SDP, *client_candidates, *repo_name, *session_id;
                            
                            json_unpack(recvd_session_JData, "{s:s, s:s, s:s, s:s}", 
                                        "client_SDP", &client_SDP,
                                        "client_candidates", &client_candidates,
                                        "repo_name", &repo_name, "session_id", &session_id);

                            /* parse client_SDP and client_candidate*/
                            char *fs_SDP;
                            struct sy_rtcdc_info_t rtcdc_info;
                            memset(&rtcdc_info, 0, sizeof(struct sy_rtcdc_info_t));
                            rtcdc_peer_connection * answerer = rtcdc_create_peer_connection((void *)upload_fs_on_channel,
                                                                                            (void *)on_candidate,
                                                                                            (void *)upload_fs_on_connect,
                                                                                            STUN_IP, 0, (void *)&rtcdc_info);
                            client_SDP = (char *)json_string_value(json_object_get(recvd_session_JData, "client_SDP"));
                            rtcdc_parse_offer_sdp(answerer, client_SDP);
                            parse_candidates(answerer, client_candidates);
                            fs_SDP = rtcdc_generate_offer_sdp(answerer);
                        
                            /* send the fs_SDP, fs_candidates */
                            json_t *sent_session_JData = json_object();
                            json_t *json_arr = json_array();
                            json_object_set_new(sent_session_JData, "metadata_type", json_integer(FS_UPLOAD_READY));
                            json_object_set_new(sent_session_JData, "session_id", json_string(session_id));
                            json_object_set_new(sent_session_JData, "fs_SDP", json_string(fs_SDP));
                            json_object_set_new(sent_session_JData, "fs_candidates", json_string(rtcdc_info.local_candidates));
                            
                            sent_data_str = json_dumps(sent_session_JData, 0);
                            
                            struct lwst_writedata_t *write_data = *write_data_ptr;
                            write_data->target_wsi = wsi;
                            write_data->type = FS_UPLOAD_READY;
                            strcpy(write_data->data, sent_data_str);
                            libwebsocket_callback_on_writable(context, wsi);

                            /* assign repo_path into rtcdc_info */
                            char repo_path[PATH_SIZE];
                            strcpy(repo_path, WAREHOUSE_PATH);
                            strcat(repo_path, repo_name);
#ifdef DEBUG_FS2
                            fprintf(stderr, "FILE_SERVER: FS_UPLOAD_READY: waiting rtcdc connection\n");
#endif
                            /* allocate a uv to run rtcdc_loop */
                            uv_loop_t *main_loop = uv_default_loop();
                            uv_work_t work;
                            work.data = (void *)answerer;
                            uv_queue_work(main_loop, &work, uv_rtcdc_loop, NULL);
                            /* TODO: when the rtcdc_loop terminate? */
                            /* TODO: free memory */
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
        2048,
    },
    { NULL, NULL, 0, 0 }
};


int main(int argc, char *argv[]){
    // initial signaling
    const char *address = "localhost";
    int port = 7681;
    signal_conn = lwst_initial(address, port, fileserver_protocols, "fileserver-protocol", NULL);
    struct libwebsocket_context *context = signal_conn->context;
    fileserver_exit = 0;
    lwst_connect(context, &fileserver_exit);
    //uv_run(main_loop, UV_RUN_DEFAULT);
    return 0;
}

