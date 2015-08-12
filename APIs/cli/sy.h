#include <stdint.h>
#include "cli_struct.h"

/* sy_init return the URI_code, which is the repository location */ 
struct sy_session_t *sy_default_session();

struct sy_diff_t *sy_default_diff();

uint8_t sy_init(struct sy_session_t *sy_session, char *repo_name, char *local_repo_path, char *apikey, char *token);

uint8_t sy_connect(struct sy_session_t *sy_session, char *URI_code, char *local_repo_path, char *apikey, char *token);

uint8_t sy_status(struct sy_session_t *sy_session, struct sy_diff_t* sy_session_diff);

uint8_t sy_upload(struct sy_session_t *sy_session, struct sy_diff_t* sy_session_diff);

uint8_t sy_download(struct sy_session_t *sy_session, struct sy_diff_t* sy_session_diff);
