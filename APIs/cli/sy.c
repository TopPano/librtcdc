#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <jansson.h>

#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <openssl/md5.h>

#include <rtcdc.h>
#include "../sy_lwst.h"
#include "../sy_rtcdc.h"
#include "cli_rtcdc.h"
#include "sy.h"

#define MD5_LINE_SIZE 32
#define LOCAL_REPO_PATH "/home/uniray/local_repo/"
#define METADATA_SERVER_IP "localhost"


typedef struct rtcdc_peer_connection rtcdc_peer_connection;
typedef struct rtcdc_data_channel rtcdc_data_channel;
static uv_loop_t *main_loop;
/* this design may be a problem */
static json_t *sent_lws_JData, *recvd_lws_JData;

static struct libwebsocket_protocols client_protocols[];

char *gen_md5(char *filename)
{
    FILE *pFile;
    pFile = fopen(filename, "rb");
    int nread;
    unsigned char *hash = (unsigned char *)calloc(1, MD5_DIGEST_LENGTH);
    MD5_CTX ctx;
    MD5_Init(&ctx);
    char buf[MD5_LINE_SIZE];
    memset(buf, 0, MD5_LINE_SIZE);
    if(pFile!=NULL)
    {
        while ((nread = fread(buf, 1, sizeof buf, pFile)) > 0)
        {    
            memset(buf, 0, MD5_LINE_SIZE);
            MD5_Update(&ctx, (const void *)buf, nread);
        }

        if (ferror(pFile)) {
            /* deal with error */
        }
        fclose(pFile);
    }
    MD5_Final(hash, &ctx);

    char *result = (char *)calloc(1, MD5_DIGEST_LENGTH*2+1);
    memset(result, 0, MD5_DIGEST_LENGTH*2+1);
    int i;
    char tmp[4];
    for(i=0;i<MD5_DIGEST_LENGTH; i++)
    {
        sprintf(tmp,"%02x",hash[i]);
        strcat(result, tmp);
    }
    free(hash);
    hash = NULL;
    return result;
}



static int callback_client(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len)
{
    struct lwst_writedata_t *write_data = (struct lwst_writedata_t *)user;
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            fprintf(stderr, "SYCLIENT: LWS_CALLBACK_CLIENT_ESTABLISHED\n");
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            fprintf(stderr, "SYCLIENT: LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n");
            break;

        case LWS_CALLBACK_CLOSED:
            fprintf(stderr, "SYCLIENT: LWS_CALLBACK_CLOSED\n");
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            {
                switch(write_data->type){
                    case(SY_INIT):
                        {
                            int lws_err = libwebsocket_write(write_data->target_wsi, (void *)write_data->data, strlen(write_data->data), LWS_WRITE_TEXT);
#ifdef DEBUG_SY            
                            if(lws_err < 0)
                                fprintf(stderr, "SYCLIENT: send SY_INIT to metadata_server fail\n");
                            else
                                fprintf(stderr, "SYCLIENT: send SY_INIT to metadata_server\n");
#endif
                            break;
                        }
                    case(SY_CONNECT):
                        {
                            int lws_err = libwebsocket_write(write_data->target_wsi, (void *)write_data->data, strlen(write_data->data), LWS_WRITE_TEXT);
#ifdef DEBUG_SY            
                            if(lws_err < 0)
                                fprintf(stderr, "SYCLIENT: send SY_CONNECT to metadata_server fail\n");
                            else
                                fprintf(stderr, "SYCLIENT: send SY_CONNECT to metadata_server\n");
#endif
                            break;
                        }

                    case(SY_STATUS):
                        {
                            int lws_err = libwebsocket_write(write_data->target_wsi, (void *)write_data->data, strlen(write_data->data), LWS_WRITE_TEXT);
#ifdef DEBUG_SY            
                            if(lws_err < 0)
                                fprintf(stderr, "SYCLIENT: send SY_STATUS to metadata_server fail\n");
                            else
                                fprintf(stderr, "SYCLIENT: send SY_STATUS to metadata_server\n");
#endif
                            break;
                        }
                    case(SY_CLEANUP):
                        {
                            int lws_err = libwebsocket_write(write_data->target_wsi, (void *)write_data->data, strlen(write_data->data), LWS_WRITE_TEXT);
#ifdef DEBUG_SY            
                            if(lws_err < 0)
                                fprintf(stderr, "SYCLIENT: send SY_CLEANUP to metadata_server fail\n");
                            else
                                fprintf(stderr, "SYCLIENT: send SY_CLEANUP to metadata_server\n");
#endif
                            break;
                        }

                    default:
                        break;
                }
               
                if(sent_lws_JData != NULL){
                    char *session_SData = json_dumps(sent_lws_JData, 0);
                    int lws_err = libwebsocket_write(wsi, (void *)session_SData, strlen(session_SData), LWS_WRITE_TEXT);
#ifdef DEBUG_SY            
                    if(lws_err<0)
                        fprintf(stderr, "SYCLIENT: LWS_WRITE failed\n");
                    else 
                        fprintf(stderr, "SYCLIENT: %s sent\n", session_SData);
#endif
                    // json_decref(sent_lws_JData);
                    sent_lws_JData = NULL;
                }
                else
                    fprintf(stderr, "SYCLIENT: sent_lws_JData is NULL\n");
                break;
            }
        case LWS_CALLBACK_CLIENT_RECEIVE:
            {
                int metadata_type ,lws_err;
                char *session_SData = (char *)in;
                json_error_t *err = NULL;
                json_t *recvd_session_JData = NULL;
                recvd_session_JData = json_loads((const char *)session_SData, JSON_DECODE_ANY, err);
                json_unpack(recvd_session_JData, "{s:i}", "metadata_type", &metadata_type);

                switch (metadata_type){
                    case SY_INIT_OK:
                        fprintf(stderr,"SYCLIENT: receive SY_INIT_OK\n");
                        /*TODO: the naming of "write_data" is not good in receive part*/
                        /*TODO: need mutex? */
                        memset(write_data->data, 0, WRITE_DATA_SIZE);
                        strcpy(write_data->data, session_SData);
                        write_data->target_wsi = NULL;
                        write_data->type = SY_INIT_OK;
                        json_decref(recvd_session_JData);
                        recvd_session_JData = NULL;
                        return -1;
                        break;
                    case SY_CONNECT_OK:
                        fprintf(stderr,"SYCLIENT: receive SY_CONNECT_OK\n");
                        /*TODO: the naming of "write_data" is not good in receive part*/
                        /*TODO: need mutex? */
                        memset(write_data->data, 0, WRITE_DATA_SIZE);
                        strcpy(write_data->data, session_SData);
                        write_data->target_wsi = NULL;
                        write_data->type = SY_CONNECT_OK;
                        json_decref(recvd_session_JData);
                        recvd_session_JData = NULL;
                        return -1;
                        break;
                    case SY_STATUS_OK:
                        fprintf(stderr,"SYCLIENT: receive SY_STATUS_OK\n");
                        /*TODO: the naming of "write_data" is not good in receive part*/
                        /*TODO: need mutex? */
                        memset(write_data->data, 0, WRITE_DATA_SIZE);
                        strcpy(write_data->data, session_SData);
                        write_data->target_wsi = NULL;
                        write_data->type = SY_STATUS_OK;
                        json_decref(recvd_session_JData);
                        recvd_session_JData = NULL;
                        return -1;
                        break;
                    case FS_UPLOAD_READY:
                        fprintf(stderr,"SYCLIENT: receive FS_UPLOAD_READY\n");
                        recvd_lws_JData = recvd_session_JData;
                        return -1;
                        break;
                    case FS_DOWNLOAD_READY:
                        fprintf(stderr,"SYCLIENT: receive FS_DOWNLOAD_READY\n");
                        recvd_lws_JData = recvd_session_JData;
                        return -1;
                        break;
                    case SY_CLEANUP_OK:
                        fprintf(stderr,"SYCLIENT: receive SY_CLEANUP_OK\n");
                        /*TODO: the naming of "write_data" is not good in receive part*/
                        /*TODO: need mutex? */
                        memset(write_data->data, 0, WRITE_DATA_SIZE);
                        strcpy(write_data->data, session_SData);
                        write_data->target_wsi = NULL;
                        write_data->type = SY_CLEANUP_OK;
                        json_decref(recvd_session_JData);
                        recvd_session_JData = NULL;
                        return -1;
                        break;
                    default:
                        break;
                }
            }
            break;
        default:
            break;
    }
    return 0;
}


static struct libwebsocket_protocols client_protocols[] = {
    {
        "client-protocol",
        callback_client,
        2048, /* TODO: the size may be a trouble */
    },
    { NULL, NULL, 0, 0 }
};


struct sy_session_t *sy_default_session()
{
    struct sy_session_t *sy_session = (struct sy_session_t *)calloc(1, sizeof(struct sy_session_t));
    return sy_session;
}


struct sy_diff_t *sy_default_diff()
{
    return (struct sy_diff_t *)calloc(1, sizeof(struct sy_diff_t));
}


uint8_t sy_init(struct sy_session_t *sy_session, char *repo_name, char *local_repo_path, char *api_key, char *token)
{
    const char *metadata_server_IP = METADATA_SERVER_IP;
    uint16_t metadata_server_port = 7681;
    uint8_t metadata_type;
    struct lwst_conn_t *metadata_conn;
    struct lwst_writedata_t *write_data = (struct lwst_writedata_t *)calloc(1, sizeof(struct lwst_writedata_t));
    metadata_conn = lwst_initial(metadata_server_IP, metadata_server_port, client_protocols, "client-protocol", (void *)write_data);
    struct libwebsocket_context *context = metadata_conn->context;
    struct libwebsocket *wsi = metadata_conn->wsi;
    volatile uint8_t client_exit = 0;
    metadata_conn->exit = &client_exit;

    /* allocate a uv to run lwst_uv_connect() */
    main_loop = uv_default_loop();
    uv_work_t work;
    work.data = (void *)metadata_conn;
    uv_queue_work(main_loop, &work, lwst_uv_connect, NULL);
    
    json_t *sent_json = json_object();
    json_object_set_new(sent_json, "metadata_type", json_integer(SY_INIT));
    json_object_set_new(sent_json, "repo_name", json_string(repo_name));
    json_object_set_new(sent_json, "api_key", json_string(api_key));
    json_object_set_new(sent_json, "token", json_string(token));

    char *sent_str = json_dumps(sent_json, 0);
    strcpy(write_data->data, sent_str);
    write_data->target_wsi = wsi;
    write_data->type = SY_INIT;
    
    usleep(100000);
    libwebsocket_callback_on_writable(context, wsi);

    /* wait for receiving SY_INIT_OK */
    /* TODO: here needs mutex */

    while(1){
        if(write_data->type == SY_INIT_OK){
            /* TODO:sy_init success, keep or close the libwebsocket connection?*/
            client_exit = 1;
            break;
        }
        usleep(1000);
    }

    uv_run(main_loop, UV_RUN_DEFAULT);

    /* extract the URI_code & session_id and assign to sy_session */
    json_t *recvd_json = json_loads((const char *)write_data->data, JSON_DECODE_ANY, NULL);
    char *session_id, *URI_code;
    json_unpack(recvd_json, "{s:s, s:s}", "session_id", &session_id, "URI_code", &URI_code);

    strcpy(sy_session->repo_name, repo_name);
    sy_session->session_id = (char *)calloc(1, strlen(session_id)+1);
    sy_session->URI_code = (char *)calloc(1, strlen(URI_code)+1);
    sy_session->local_repo_path = (char *)calloc(1, strlen(local_repo_path)+1);
    strcpy(sy_session->session_id, session_id);
    strcpy(sy_session->URI_code, URI_code);
    strcpy(sy_session->local_repo_path, local_repo_path);

    /* free memory */
    free(sent_str);
    sent_str = NULL;
    libwebsocket_context_destroy(context);
    free(write_data);
    write_data = NULL;
    
    session_id = NULL;
    URI_code = NULL;
    
    json_decref(recvd_json);
    json_decref(sent_json); 
    free(metadata_conn);
    /* return METADATAtype */
    return SY_INIT_OK;
}


uint8_t sy_connect(struct sy_session_t *sy_session, char *URI_code, char *local_repo_path, char *api_key, char *token)
{

    const char *metadata_server_IP = METADATA_SERVER_IP;
    uint16_t metadata_server_port = 7681;
    uint8_t metadata_type;
    char *session_id;
    struct lwst_conn_t *metadata_conn;
    struct lwst_writedata_t *write_data = (struct lwst_writedata_t *)calloc(1, sizeof(struct lwst_writedata_t));
    metadata_conn = lwst_initial(metadata_server_IP, metadata_server_port, client_protocols, "client-protocol", (void *)write_data);

    struct libwebsocket_context *context = metadata_conn->context;
    struct libwebsocket *wsi = metadata_conn->wsi;
    volatile uint8_t client_exit = 0;
    metadata_conn->exit = &client_exit;

    /* allocate a uv to run lwst_uv_connect() */
    main_loop = uv_default_loop();
    uv_work_t work;
    work.data = (void *)metadata_conn;
    uv_queue_work(main_loop, &work, lwst_uv_connect, NULL);
    
    json_t *sent_json = json_object();
    json_object_set_new(sent_json, "metadata_type", json_integer(SY_CONNECT));
    json_object_set_new(sent_json, "URI_code", json_string(URI_code));
    json_object_set_new(sent_json, "api_key", json_string(api_key));
    json_object_set_new(sent_json, "token", json_string(token));

    char *sent_str = json_dumps(sent_json, 0);
    strcpy(write_data->data, sent_str);
    write_data->target_wsi = wsi;
    write_data->type = SY_CONNECT;

    usleep(100000);
    libwebsocket_callback_on_writable(context, wsi);

    /* wait for receiving SY_CONNECT_OK */
    /* TODO:here needs mutex */
    while(1){
        if(write_data->type == SY_CONNECT_OK){
            /* TODO:sy_connect success, keep or close the libwebsocket connection?*/
            client_exit = 1;
            break;
        }
        usleep(1000);
    }
    uv_run(main_loop, UV_RUN_DEFAULT);
    
    /* extract the URI_code & session_id and assign to sy_session */
    json_t *recvd_json = json_loads((const char *)write_data->data ,JSON_DECODE_ANY, NULL);
    json_unpack(recvd_json, "{s:s}", "session_id", &(session_id));
    json_unpack(recvd_json, "{s:s}", "URI_code", &(URI_code));

    sy_session->session_id = (char *)calloc(1, strlen(session_id)+1);
    sy_session->URI_code = (char *)calloc(1, strlen(URI_code)+1);
    sy_session->local_repo_path = (char *)calloc(1, strlen(local_repo_path)+1);
    strcpy(sy_session->session_id, session_id);
    strcpy(sy_session->URI_code, URI_code);
    strcpy(sy_session->local_repo_path, local_repo_path);

    /* free memory */
    free(sent_str);
    sent_str = NULL;
    libwebsocket_context_destroy(context);
    free(write_data);
    write_data = NULL;

    json_decref(recvd_json);
    json_decref(sent_json); 

    free(metadata_conn);
    /* return METADATAtype */
    return SY_CONNECT_OK;

}

uint8_t sy_status(struct sy_session_t *sy_session, struct sy_diff_t *sy_session_diff)
{
    const char *metadata_server_IP = METADATA_SERVER_IP;
    uint16_t metadata_server_port = 7681;
    uint8_t metadata_type;
    struct lwst_conn_t *metadata_conn;
    struct lwst_writedata_t *write_data = (struct lwst_writedata_t *)calloc(1, sizeof(struct lwst_writedata_t));
    metadata_conn = lwst_initial(metadata_server_IP, metadata_server_port, client_protocols, "client-protocol", (void *)write_data);
    struct libwebsocket_context *context = metadata_conn->context;
    struct libwebsocket *wsi = metadata_conn->wsi;
    volatile uint8_t client_exit = 0;
    metadata_conn->exit = &client_exit;

    /* allocate a uv to run lwst_uv_connect() */
    main_loop = uv_default_loop();
    uv_work_t work;
    work.data = (void *)metadata_conn;
    uv_queue_work(main_loop, &work, lwst_uv_connect, NULL);

    /* construct sent_json and calculate md5 of each files */
    json_t *sent_json, *sent_file_array_json;
    sent_json = json_object();
    sent_file_array_json = json_array();
    json_object_set_new(sent_json, "metadata_type", json_integer(SY_STATUS));
    json_object_set_new(sent_json, "session_id", json_string(sy_session->session_id));
    json_object_set_new(sent_json, "files", sent_file_array_json);

    /* read all files in local_repo_path*/
    DIR *dp;
    struct dirent *ep;
    dp = opendir(sy_session->local_repo_path);

    if (dp != NULL)
    {
        while (ep = readdir (dp))
        {    
            if(strcmp((const char*)ep->d_name, ".") && strcmp((const char*)ep->d_name, "..") != 0)
            {
                json_t *file_json = json_object();
                /* calculate the file checksum and put it in json object */
                char file_path[128];
                strcpy(file_path, LOCAL_REPO_PATH);
                strcat(file_path, ep->d_name);
                char *file_md5 = gen_md5(file_path);
                json_object_set_new(file_json, "filename", json_string(ep->d_name));
                json_object_set_new(file_json, "client_checksum", json_string(file_md5));
                json_array_append(sent_file_array_json, file_json);
                json_decref(file_json);
                free(file_md5);
                file_md5 = NULL;
            }
        }
        (void) closedir (dp);
    }
    else
        perror ("Couldn't open the directory");

    /* send the json object throw libwebsocket */
    char *sent_str = json_dumps(sent_json, 0);
    strcpy(write_data->data, sent_str);
    write_data->target_wsi = wsi;
    write_data->type = SY_STATUS;

    usleep(100000);
    libwebsocket_callback_on_writable(context, wsi);

    /* wait for message */
    /* TODO:here needs mutex */
    while(1){
        if(write_data->type == SY_STATUS_OK){
            /* TODO:sy_status success, keep or close the libwebsocket connection?*/
            client_exit = 1;
            break;
        }
        usleep(1000);
    }

    uv_run(main_loop, UV_RUN_DEFAULT);
    /* construct DIFF structure and return */
    json_t *recvd_json, *recvd_file_array_json, *recvd_file_element_json;
    struct diff_info_t *files_diff; 
    
    recvd_json = json_loads((const char *)write_data->data, JSON_DECODE_ANY, NULL); 
    recvd_file_array_json = json_object_get(recvd_json, "files");
    recvd_file_element_json;
    size_t file_array_size = json_array_size(recvd_file_array_json);
    size_t file_array_index;
    int dirty;
    char *filename;
    files_diff = (struct diff_info_t *)calloc(file_array_size+1, sizeof(struct diff_info_t)); 
    json_array_foreach(recvd_file_array_json, file_array_index, recvd_file_element_json)
    {
        json_unpack(recvd_file_element_json, "{s:s, s:i}", "filename", &filename, "dirty", &dirty);
        strcpy(files_diff[file_array_index].filename, filename); 
        files_diff[file_array_index].dirty = dirty;
    }
    sy_session_diff->files_diff = files_diff;
    sy_session_diff->num = file_array_size;

    /* free memory */
    free(sent_str);
    sent_str = NULL;
    libwebsocket_context_destroy(context);
    free(write_data);
    write_data = NULL;

    json_decref(sent_file_array_json);
    /* recvd_file_element_json is a pointer to a part of recvd_file_array_json */
    //json_decref(recvd_file_element_json);
    json_decref(recvd_file_array_json);
    json_decref(recvd_json);
    json_decref(sent_json);

    free(metadata_conn);
    metadata_conn = NULL;
    /* return METADATAtype */
    return SY_STATUS_OK;
}

uint8_t sy_upload(struct sy_session_t *sy_session, struct sy_diff_t *sy_session_diff)
{
    /* connect to metadata server*/
    const char *metadata_server_IP = METADATA_SERVER_IP;
    uint16_t metadata_server_port = 7681;
    uint8_t metadata_type;
    char *session_id, *URI_code;
    struct lwst_conn_t *metadata_conn;
    metadata_conn = lwst_initial(metadata_server_IP, metadata_server_port, client_protocols, "client-protocol", NULL);
    struct libwebsocket_context *context = metadata_conn->context;
    struct libwebsocket *wsi = metadata_conn->wsi;
    volatile uint8_t client_exit = 0;
    metadata_conn->exit = &client_exit;

    /* allocate a uv to run lwst_uv_connect() */
    main_loop = uv_default_loop();
    uv_work_t work;
    work.data = (void *)metadata_conn;
    uv_queue_work(main_loop, &work, lwst_uv_connect, NULL);

    /* gen rtcdc SDP and candidates */
    char *client_SDP;
    struct sy_rtcdc_info_t rtcdc_info;
    memset(&rtcdc_info, 0, sizeof(struct sy_rtcdc_info_t));

    struct rtcdc_peer_connection *offerer = rtcdc_create_peer_connection((void *)upload_client_on_channel, (void *)on_candidate, 
            (void *)upload_client_on_connect,STUN_IP, 0, 
            (void *)&rtcdc_info);
    client_SDP = rtcdc_generate_offer_sdp(offerer);
    /* send request: upload files which the fileserver doesnt have */
    /* write SY_UPLOAD, SDP, candidate and session_id */
    sent_lws_JData = json_object();
    json_object_set_new(sent_lws_JData, "metadata_type", json_integer(SY_UPLOAD));
    json_object_set_new(sent_lws_JData, "session_id", json_string(sy_session->session_id));
    json_object_set_new(sent_lws_JData, "client_SDP", json_string(client_SDP));
    json_object_set_new(sent_lws_JData, "client_candidates", json_string(rtcdc_info.local_candidates));
    recvd_lws_JData = NULL;

    usleep(100000);
    libwebsocket_callback_on_writable(context, wsi);

    /* wait for SY_UPLOAD_READY fs SDP and candidate */
    /* TODO: here needs mutex */
    json_t *sy_upload_json;
    while(1){
        if(recvd_lws_JData != NULL){
            json_unpack(recvd_lws_JData, "{s:i}", "metadata_type", &metadata_type);
            if(metadata_type == FS_UPLOAD_READY){
                /* TODO:sy_init success, keep or close the libwebsocket connection?*/
                sy_upload_json = json_deep_copy(recvd_lws_JData); 
                client_exit = 1;
                break;
            }
        }
        usleep(1000);
    }
    /* close the libwebsocket connection and then start rtcdc connection*/
    /* parse fs_SDP and fs_candidates */
    char *fs_SDP = (char *)json_string_value(json_object_get(sy_upload_json, "fs_SDP"));
    char *fs_candidates = (char *)json_string_value(json_object_get(sy_upload_json, "fs_candidates"));

    rtcdc_parse_offer_sdp(offerer, fs_SDP);
    parse_candidates(offerer, fs_candidates);

    /* pass the rtcdc_info data*/
    strncpy(rtcdc_info.local_repo_path, LOCAL_REPO_PATH, strlen(LOCAL_REPO_PATH));
    rtcdc_info.sy_session_diff = sy_session_diff;
    rtcdc_info.peer = offerer;
    rtcdc_info.uv_loop = main_loop; 
    /* rtcdc connection */
    rtcdc_loop(offerer);
    /* TODO: when the rtcdc_loop terminate? */
    uv_run(main_loop, UV_RUN_DEFAULT);
    /* free memory */
    free(client_SDP);
    free(recvd_lws_JData);
    free(sent_lws_JData); 
    recvd_lws_JData = NULL;
    sent_lws_JData = NULL;

    free(metadata_conn);
    free(sent_lws_JData);
    /* return METADATAtype */
    return SY_UPLOAD_OK;
}


uint8_t sy_download(struct sy_session_t *sy_session, struct sy_diff_t *sy_session_diff)
{
    /* connect to metadata server*/
    const char *metadata_server_IP = METADATA_SERVER_IP;
    uint16_t metadata_server_port = 7681;
    uint8_t metadata_type;
    char *session_id, *URI_code;
    struct lwst_conn_t *metadata_conn;
    metadata_conn = lwst_initial(metadata_server_IP, metadata_server_port, client_protocols, "client-protocol", NULL);
    struct libwebsocket_context *context = metadata_conn->context;
    struct libwebsocket *wsi = metadata_conn->wsi;
    volatile uint8_t client_exit = 0;
    metadata_conn->exit = &client_exit;

    /* allocate a uv to run lwst_uv_connect() */
    main_loop = uv_default_loop();
    uv_work_t work;
    work.data = (void *)metadata_conn;
    uv_queue_work(main_loop, &work, lwst_uv_connect, NULL);

    /* gen rtcdc SDP and candidates */
    char *client_SDP;
    struct sy_rtcdc_info_t rtcdc_info;
    memset(&rtcdc_info, 0, sizeof(struct sy_rtcdc_info_t));

    struct rtcdc_peer_connection *offerer = rtcdc_create_peer_connection((void *)download_client_on_channel, (void *)on_candidate, 
            (void *)download_client_on_connect,STUN_IP, 0, 
            (void *)&rtcdc_info);
    client_SDP = rtcdc_generate_offer_sdp(offerer);
    /* send request: download files which the client doesnt have */
    /* write SY_DOWNLOAD, SDP, candidate and session_id */
    sent_lws_JData = json_object();
    json_object_set_new(sent_lws_JData, "metadata_type", json_integer(SY_DOWNLOAD));
    json_object_set_new(sent_lws_JData, "session_id", json_string(sy_session->session_id));
    json_object_set_new(sent_lws_JData, "client_SDP", json_string(client_SDP));
    json_object_set_new(sent_lws_JData, "client_candidates", json_string(rtcdc_info.local_candidates));
    recvd_lws_JData = NULL;

    usleep(100000);
    libwebsocket_callback_on_writable(context, wsi);

    /* wait for SY_DOWNLOAD_READY fs SDP and candidate */
    /* TODO: here needs mutex */
    json_t *sy_download_json;
    while(1){
        if(recvd_lws_JData != NULL){
            json_unpack(recvd_lws_JData, "{s:i}", "metadata_type", &metadata_type);
            if(metadata_type == FS_DOWNLOAD_READY){
                /* TODO:sy_init success, keep or close the libwebsocket connection?*/
                sy_download_json = json_deep_copy(recvd_lws_JData); 
                client_exit = 1;
                break;
            }
        }
        usleep(1000);
    }
    /* close the libwebsocket connection and then start rtcdc connection*/
    /* parse fs_SDP and fs_candidates */
    char *fs_SDP = (char *)json_string_value(json_object_get(sy_download_json, "fs_SDP"));
    char *fs_candidates = (char *)json_string_value(json_object_get(sy_download_json, "fs_candidates"));

    rtcdc_parse_offer_sdp(offerer, fs_SDP);
    parse_candidates(offerer, fs_candidates);
    
    /* pass the rtcdc_info data*/
    strncpy(rtcdc_info.local_repo_path, LOCAL_REPO_PATH, strlen(LOCAL_REPO_PATH));
    rtcdc_info.sy_session_diff = sy_session_diff;
    rtcdc_info.peer = offerer;
    rtcdc_info.uv_loop = main_loop; 
    /* rtcdc connection */
    rtcdc_loop(offerer);
    /* TODO: when the rtcdc_loop terminate? */
    uv_run(main_loop, UV_RUN_DEFAULT);
    /* free memory */
    free(client_SDP);
    json_decref(sy_download_json);
    json_decref(recvd_lws_JData);
    json_decref(sent_lws_JData); 
    recvd_lws_JData = NULL;
    sent_lws_JData = NULL;

    free(metadata_conn);
    free(sent_lws_JData);
    /* return METADATAtype */
    return SY_DOWNLOAD_OK;
}


uint8_t sy_cleanup(struct sy_session_t *sy_session, struct sy_diff_t *sy_session_diff)
{
    /* connect to metadata server*/
    const char *metadata_server_IP = METADATA_SERVER_IP;
    uint16_t metadata_server_port = 7681;
    uint8_t metadata_type;
    struct lwst_conn_t *metadata_conn;
    struct lwst_writedata_t *write_data = (struct lwst_writedata_t *)calloc(1, sizeof(struct lwst_writedata_t));
    metadata_conn = lwst_initial(metadata_server_IP, metadata_server_port, client_protocols, "client-protocol", (void *)write_data);
    struct libwebsocket_context *context = metadata_conn->context;
    struct libwebsocket *wsi = metadata_conn->wsi;
    volatile uint8_t client_exit = 0;
    metadata_conn->exit = &client_exit;

    /* allocate a uv to run lwst_uv_connect() */
    main_loop = uv_default_loop();
    uv_work_t work;
    work.data = (void *)metadata_conn;
    uv_queue_work(main_loop, &work, lwst_uv_connect, NULL);
    
    /* send request to metadata server: cleanup the session */
    /* write SY_CLEANUP and session_id */
    json_t *sent_json = json_object();
    json_object_set_new(sent_json, "metadata_type", json_integer(SY_CLEANUP));
    json_object_set_new(sent_json, "session_id", json_string(sy_session->session_id));

    char *sent_str = json_dumps(sent_json, 0);
    strcpy(write_data->data, sent_str);
    write_data->target_wsi = wsi;
    write_data->type = SY_CLEANUP;

    usleep(100000);
    libwebsocket_callback_on_writable(context, wsi);

    /* wait for SY_CLEANUP_OK */
    /* TODO: here needs mutex */
    while(1){
        if(write_data->type == SY_CLEANUP_OK){
            /* TODO:sy_init success, keep or close the libwebsocket connection?*/
            client_exit = 1;
            break;
        }
        usleep(1000);
    }
    
    uv_run(main_loop, UV_RUN_DEFAULT);
    
    /* free memory: sy_session and sy_session_diff */
    if(sy_session != NULL)
    {
        free(sy_session->session_id);
        free(sy_session->URI_code);
        //free(sy_session->wsi);
        //libwebsocket_context_destroy(sy_session->context);
        free(sy_session->local_repo_path);
        free(sy_session);
        sy_session = NULL;
    }

    if(sy_session_diff != NULL)
    {
        free(sy_session_diff->files_diff);
        free(sy_session_diff);
        sy_session_diff = NULL;
    }

    /* free memory */
    free(sent_str);
    sent_str = NULL;
    free(write_data);
    write_data = NULL;
    libwebsocket_context_destroy(context);
    
    json_decref(sent_json);
    sent_json = NULL;
    free(metadata_conn);
    metadata_conn = NULL;
    return SY_CLEANUP_OK;
}


int main(int argc, char *argv[]){

    struct sy_session_t *sy_session = sy_default_session();
    char *repo_name = "hhh";
    char local_repo_path[128];
    strcpy(local_repo_path, LOCAL_REPO_PATH);
    if(sy_init(sy_session, repo_name, local_repo_path, "apikey", "token") == SY_INIT_OK)
        fprintf(stderr, "sy_init finish\nsession_id:%s, URI_code:%s\n", sy_session->session_id, sy_session->URI_code);
    else
        fprintf(stderr, "sy_init failed\n");

    char *URI_code = strdup(sy_session->URI_code);
    sy_cleanup(sy_session, NULL);
    

    sy_session = sy_default_session();
    if(sy_connect(sy_session, URI_code, local_repo_path, "apikey", "token") == SY_CONNECT_OK)
        fprintf(stderr, "sy_connect finish\nsession_id:%s, URI_code:%s local_repo_path: %s\n", 
                        sy_session->session_id, sy_session->URI_code, sy_session->local_repo_path);
    else
        fprintf(stderr, "sy_connect failed");
    
    
    struct sy_diff_t *sy_session_diff = sy_default_diff();
    sy_status(sy_session, sy_session_diff);

    int i;
    for(i=0;i<sy_session_diff->num;i++)
    {
        printf("filename:%s, dirty:%d\n", (sy_session_diff->files_diff[i]).filename, (sy_session_diff->files_diff[i]).dirty);
    }
    printf("\n");

    sy_cleanup(sy_session, sy_session_diff);
    
    free(URI_code);
    URI_code = NULL;
    return 0;
}

