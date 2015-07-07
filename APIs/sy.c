#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <jansson.h>
#include <rtcdc.h>
#include "signaling.h"
#include "sy.h"
typedef struct rtcdc_peer_connection rtcdc_peer_connection;
typedef struct rtcdc_data_channel rtcdc_data_channel;
static uv_loop_t *main_loop;
/* this design may be a problem */
static json_t *sent_lws_JData, *recvd_lws_JData;

static struct libwebsocket_protocols client_protocols[];

static int callback_client(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len)
{
    struct signal_session_data_t *sent_session_data = NULL;
    struct signal_session_data_t *recvd_session_data = NULL;
    int result;
    char *client_SDP = NULL;
    char *client_candidates = NULL;

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
                if(sent_lws_JData != NULL){
                    char *session_SData = json_dumps(sent_lws_JData, 0);
                    int lws_err = libwebsocket_write(wsi, (void *)session_SData, strlen(session_SData), LWS_WRITE_TEXT);
#ifdef DEBUG_SY            
                    if(lws_err<0)
                        fprintf(stderr, "SYCLIENT: LWS_WRITE failed\n");
                    else 
                        fprintf(stderr, "SYCLIENT: %s sent\n", session_SData);
#endif
                    json_decref(sent_lws_JData);
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
                        recvd_lws_JData = recvd_session_JData;
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
        sizeof(struct signal_session_data_t), /* TODO: the size may be a trouble */
        10,
    },
    { NULL, NULL, 0, 0 }
};


struct sy_session_t *sy_default_session()
{
    struct sy_session_t *sy_session = (struct sy_session_t *)calloc(1, sizeof(struct sy_session_t));
    return sy_session;
}


uint8_t sy_init(struct sy_session_t *sy_session, char *repo_name, char *local_repo_path, char *api_key, char *token)
{
    const char *metadata_server_IP = "140.112.90.37";
    uint16_t metadata_server_port = 7681;
    uint8_t metadata_type;
    char *session_id, *URI_code;
    struct conn_info_t *metadata_conn;
    metadata_conn = signal_initial(metadata_server_IP, metadata_server_port, client_protocols, "client-protocol");
    struct libwebsocket_context *context = metadata_conn->context;
    struct libwebsocket *wsi = metadata_conn->wsi;
    volatile uint8_t client_exit = 0;
    metadata_conn->exit = &client_exit;

    /* allocate a uv to run signal_connect() */
    main_loop = uv_default_loop();
    uv_work_t work;
    work.data = (void *)metadata_conn;
    uv_queue_work(main_loop, &work, uv_signal_connect, NULL);
    sent_lws_JData = json_object();
    json_object_set_new(sent_lws_JData, "metadata_type", json_integer(SY_INIT));
    json_object_set_new(sent_lws_JData, "repo_name", json_string(repo_name));
    json_object_set_new(sent_lws_JData, "api_key", json_string(api_key));
    json_object_set_new(sent_lws_JData, "token", json_string(token));

    recvd_lws_JData = NULL;

    usleep(100000);
    libwebsocket_callback_on_writable(context, wsi);

    /* wait for receiving SY_INIT_OK */
    while(1){
        if(recvd_lws_JData != NULL){
            json_unpack(recvd_lws_JData, "{s:i}", "metadata_type", &metadata_type);
            if(metadata_type == SY_INIT_OK){
                /* TODO:sy_init success, keep or close the libwebsocket connection?*/
                client_exit = 1;
                break;
            }
        }
        usleep(1000);
    }
    /* extract the URI_code & session_id and assign to sy_session */

    /* construct sy_session */
    sy_session->wsi = wsi;
    sy_session->context = context;
    strcpy(sy_session->repo_name, repo_name);

    json_unpack(recvd_lws_JData, "{s:s}", "session_id", &(session_id));
    json_unpack(recvd_lws_JData, "{s:s}", "URI_code", &(URI_code));

    sy_session->session_id = (char *)calloc(1, strlen(session_id));
    sy_session->URI_code = (char *)calloc(1, strlen(URI_code));
    strcpy(sy_session->session_id, session_id);
    strcpy(sy_session->URI_code, URI_code);
    
    /* free memory */
    free(recvd_lws_JData);
    free(sent_lws_JData); 
    recvd_lws_JData = NULL;
    sent_lws_JData = NULL;
    uv_run(main_loop, UV_RUN_DEFAULT);

    /* return METADATAtype */
    return SY_INIT_OK;
}


uint8_t sy_connect(struct sy_session_t *sy_session, char *URI_code, char *apikey, char *token)
{
    
}

uint8_t sy_upload(struct sy_session_t *sy_session)
{

}



int main(int argc, char *argv[]){

    struct sy_session_t *sy_session = sy_default_session();
    char *repo_name = "hhh";
    sy_init(sy_session, repo_name, "apikey", "token");
    
    fprintf(stderr, "sy_init finish\nsession_id:%s, URI_code:%s\n", sy_session->session_id, sy_session->URI_code);

    return 0;
}

