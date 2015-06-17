

#define SDP_SIZE 1024

typedef enum {
    INIT_OK = 0,
    INIT_AUTH_FAIL,
    CONNECT_OK,
    CONNECT_AUTH_FAIL, 
} SYcode;


typedef struct sy_session{


} sy_session_t;


typedef struct{
    char sdp[SDP_SIZE];
    char candidates[SDP_SIZE];
} SDP_t;


SYcode sy_init(sy_session_t *session, char *repo_name, char *apikey, char *token);

SYcode sy_connect(sy_session_t *session, char *repo_name, char *apikey, char *token);
