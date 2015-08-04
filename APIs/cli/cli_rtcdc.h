#include <rtcdc.h>
#include <uv.h>

#define SDP_SIZE 2048
#define PATH_SIZE 128
#define STUN_IP "stun.services.mozilla.com"

struct sy_rtcdc_info_t{
    char local_candidates[SDP_SIZE];
    char local_repo_path[PATH_SIZE];
    /* filename */
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

