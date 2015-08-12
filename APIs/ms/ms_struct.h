#include <libwebsockets.h>
#include "../common.h"

/* TODO: what's DATASIZE?*/
#define DATASIZE 1024

struct ms_URI_info_t{
    struct libwebsocket *fileserver_wsi;
    char repo_name[REPO_NAME_SIZE];
    char URI_code[ID_SIZE];
};


struct ms_session_info_t{
    struct libwebsocket *fileserver_wsi;
    struct libwebsocket *client_wsi;
    char repo_name[REPO_NAME_SIZE];
    char session_id[ID_SIZE];
    char files[DATASIZE];
    char diff[DATASIZE];
    METADATAtype state;
};


