#ifndef _SY_RTCDC_H_
#define _SY_RTCDC_H_

#include <uv.h>
#include <rtcdc.h>
#include "common.h"

#define STUN_IP "stun.services.mozilla.com"

#define SCTP_BUF_SIZE  1208

#ifndef SDP_SIZE
#define SDP_SIZE 1400
#endif 


struct sctp_packet_t{
    int index;
    char buf[SCTP_BUF_SIZE];
    int buf_len;
};

struct sy_rtcdc_info_t{
    char local_candidates[SDP_SIZE];
    char local_repo_path[LOCAL_PATH_SIZE];
    const struct sy_diff_t *sy_session_diff;
    struct rtcdc_data_channel *data_channel;
    uv_loop_t *uv_loop;
};

void on_candidate(struct rtcdc_peer_connection *peer, 
        char *candidate, 
        void *user_data);

void uv_rtcdc_loop(uv_work_t *work);

void parse_candidates(struct rtcdc_peer_connection *peer, char *candidates);


#endif // _SY_RTCDC_H_
