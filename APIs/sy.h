#include <stdio.h>
#include <stdint.h>
#include <libwebsockets.h>
#include <rtcdc.h>
#define NAMESIZE 32

struct sy_conn_info_t{
    char repo_name[NAMESIZE];
    struct rtcdc_peer_connection *peer;
};

struct sy_session_t{
    char *session_id;
    char *URI_code;
    struct libwebsocket *wsi;
    struct libwebsocket_context *context;
    char *local_repo_path;
    char repo_name[NAMESIZE];
};

/* sy_init return the URI_code, which is the repository location */ 
struct sy_session_t *sy_default_session();

uint8_t sy_init(struct sy_session_t *sy_session, char *repo_name, char *local_repo_path, char *apikey, char *token);

uint8_t sy_connect(struct sy_session_t *sy_session, char *URI_code, char *local_repo_path, char *apikey, char *token);

uint8_t sy_upload(struct sy_session_t *sy_session);
