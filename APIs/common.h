#ifndef _SY_COMMON_H_
#define _SY_COMMON_H_

#ifndef WRITE_DATA_SIZE
#define WRITE_DATA_SIZE 2048
#endif 

#ifndef REPO_NAME_SIZE
#define REPO_NAME_SIZE 64
#endif 

#ifndef ID_SIZE
#define ID_SIZE 32
#endif 


typedef enum {
    FS_REGISTER_t = 0,
    FS_REGISTER_OK_t,
    FS_REPO_EXIST,
    FS_INIT_OK,
    FS_REPO_NOT_EXIST,
    FS_CONNECT_OK,
    FS_STATUS_OK,
    FS_UPLOAD_READY,
    FS_UPLOAD_OK,
    FS_DOWNLOAD_OK,
    SY_INIT,
    SY_REPO_EXIST,
    SY_INIT_OK,
    SY_OAUTH,
    SY_OAUTH_OK,
    SY_OAUTH_FAIL,
    SY_CONNECT,
    SY_REPO_NOT_EXIST,
    SY_CONNECT_OK,
    SY_STATUS,
    SY_STATUS_OK,
    SY_UPLOAD,
    SY_UPLOAD_OK,
    SY_DOWNLOAD,
    SY_DOWNLOAD_OK,
    SY_SESSION_NOT_EXIST,
    SY_SDP,
    FS_SDP,
} METADATAtype;


typedef enum {
    FILE_CLEAN = 0,
    FILE_CLIENT_LACK,
    FILE_FS_LACK,
    FILE_DIRTY,
} FILEstatus;


#endif // _SY_COMMON_H_
