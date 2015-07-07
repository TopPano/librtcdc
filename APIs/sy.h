#include <stdio.h>
#include <stdint.h>
#include <libwebsockets.h>
#include <rtcdc.h>
#define NAMESIZE 32
/*
typedef enum {
    FS_REGISTER_t = 0,
    FS_REGISTER_OK_t,
    FS_INIT_OK,
    FS_CONNECT_OK,
    FS_STATUS_OK,
    FS_UPLOAD_OK,
    FS_DOWNLOAD_OK,
    SY_INIT,
    SY_INIT_OK,
    SY_OAUTH,
    SY_OAUTH_OK,
    SY_CONNECT,
    SY_CONNECT_OK,
    SY_STATUS,
    SY_STATUS_OK,
    SY_UPLOAD,
    SY_UPLOAD_OK,
    SY_DOWNLOAD,
    SY_DOWNLOAD_OK
} METADATAtype;
*/

struct sy_conn_info_t{
    char repo_name[NAMESIZE];
    struct rtcdc_peer_connection *peer;
};

struct sy_session_t{
    char *session_id;
    char *URI_code;
    struct libwebsocket *wsi;
    struct libwebsocket_context *context;
    char repo_name[NAMESIZE];
};

/* sy_init return the URI_code, which is the repository location */ 
struct sy_session_t *sy_default_session();
uint8_t sy_init(struct sy_session_t *sy_session, char *repo_name, char *apikey, char *token);
void sy_connect(struct sy_session_t *sy_session, uint8_t URI_code, char *apikey, char *token);
