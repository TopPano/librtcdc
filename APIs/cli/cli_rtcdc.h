#include <rtcdc.h>
#include <uv.h>
#include "cli_struct.h"
#include "../common.h"

#define STUN_IP "stun.services.mozilla.com"

struct sy_rtcdc_info_t{
    char local_candidates[SDP_SIZE];
    char local_repo_path[LOCAL_PATH_SIZE];
    const struct sy_diff_t *sy_session_diff;
};

void on_candidate(struct rtcdc_peer_connection *peer, 
                    char *candidate, 
                    void *user_data);


void upload_client_on_channel(struct rtcdc_peer_connection *peer,
                struct rtcdc_data_channel *dc, 
                void *user_data);


void upload_client_on_connect(struct rtcdc_peer_connection *peer,
                void *user_data);


void uv_rtcdc_loop(uv_work_t *work);

void parse_candidates(struct rtcdc_peer_connection *peer, char *candidates);

