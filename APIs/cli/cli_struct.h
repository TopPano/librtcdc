#ifndef _SY_CLI_STRUCT_H_
#define _SY_CLI_STRUCT_H_

#include "../common.h"

struct sy_session_t{
    char *session_id;
    char *URI_code;
    struct libwebsocket *wsi;
    struct libwebsocket_context *context;
    char *local_repo_path;
    char repo_name[REPO_NAME_SIZE];
};


struct sy_diff_t{
    uint8_t num;
    struct diff_info_t *files_diff;
};


struct diff_info_t{
    char filename[FILE_NAME_SIZE];
    FILEstatus dirty;
    /* 1 is dirty, while 0 is not */
};

#endif //_SY_CLI_STRUCT_H_
