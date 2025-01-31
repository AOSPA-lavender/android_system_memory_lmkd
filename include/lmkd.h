/*
 *  Copyright 2018 Google, Inc
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef _LMKD_H_
#define _LMKD_H_

#include <arpa/inet.h>
#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

/*
 * Supported LMKD commands
 */
enum lmk_cmd {
    LMK_TARGET = 0,         /* Associate minfree with oom_adj_score */
    LMK_PROCPRIO,           /* Register a process and set its oom_adj_score */
    LMK_PROCREMOVE,         /* Unregister a process */
    LMK_PROCPURGE,          /* Purge all registered processes */
    LMK_GETKILLCNT,         /* Get number of kills */
    LMK_SUBSCRIBE,          /* Subscribe for asynchronous events */
    LMK_PROCKILL,           /* Unsolicited msg to subscribed clients on proc kills */
    LMK_UPDATE_PROPS,       /* Reinit properties */
    LMK_STAT_KILL_OCCURRED, /* Unsolicited msg to subscribed clients on proc kills for statsd log */
    LMK_START_MONITORING,   /* Start psi monitoring if it was skipped earlier */
    LMK_BOOT_COMPLETED,     /* Notify LMKD boot is completed */
    LMK_PROCS_PRIO,         /* Register processes and set the same oom_adj_score */
};

/*
 * Max number of targets in LMK_TARGET command.
 */
#define MAX_TARGETS 6

/*
 * Max packet length in bytes.
 * Longest packet is LMK_TARGET followed by MAX_TARGETS
 * of minfree and oom_adj_score values
 */
#define CTRL_PACKET_MAX_SIZE (sizeof(int) * (MAX_TARGETS * 2 + 1))

/*
 * Max size of work buffer
 */
#define BUF_MAX 4096

/* LMKD packet - first int is lmk_cmd followed by payload */
typedef int LMKD_CTRL_PACKET[CTRL_PACKET_MAX_SIZE / sizeof(int)];

/* Get LMKD packet command */
static inline enum lmk_cmd lmkd_pack_get_cmd(LMKD_CTRL_PACKET pack) {
    return (enum lmk_cmd)ntohl(pack[0]);
}

/* LMK_TARGET packet payload */
struct lmk_target {
    int minfree;
    int oom_adj_score;
};

/*
 * For LMK_TARGET packet get target_idx-th payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline void lmkd_pack_get_target(LMKD_CTRL_PACKET packet, int target_idx,
                                        struct lmk_target* target) {
    target->minfree = ntohl(packet[target_idx * 2 + 1]);
    target->oom_adj_score = ntohl(packet[target_idx * 2 + 2]);
}

/*
 * Prepare LMK_TARGET packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_target(LMKD_CTRL_PACKET packet, struct lmk_target* targets,
                                          size_t target_cnt) {
    int idx = 0;
    packet[idx++] = htonl(LMK_TARGET);
    while (target_cnt) {
        packet[idx++] = htonl(targets->minfree);
        packet[idx++] = htonl(targets->oom_adj_score);
        targets++;
        target_cnt--;
    }
    return idx * sizeof(int);
}

/* Process types for lmk_procprio.ptype */
enum proc_type {
    PROC_TYPE_FIRST,
    PROC_TYPE_APP = PROC_TYPE_FIRST,
    PROC_TYPE_SERVICE,
    PROC_TYPE_COUNT,
};

/* LMK_PROCPRIO packet payload */
struct lmk_procprio {
    pid_t pid;
    uid_t uid;
    int oomadj;
    enum proc_type ptype;
};
#define LMK_PROCPRIO_FIELD_COUNT 4
#define LMK_PROCPRIO_SIZE (LMK_PROCPRIO_FIELD_COUNT * sizeof(int))

/*
 * For LMK_PROCPRIO packet get its payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline void lmkd_pack_get_procprio(LMKD_CTRL_PACKET packet, int field_count,
                                          struct lmk_procprio* params) {
    params->pid = (pid_t)ntohl(packet[1]);
    params->uid = (uid_t)ntohl(packet[2]);
    params->oomadj = ntohl(packet[3]);
    /* if field is missing assume PROC_TYPE_APP for backward compatibility */
    params->ptype = field_count > 3 ? (enum proc_type)ntohl(packet[4]) : PROC_TYPE_APP;
}

/*
 * Prepare LMK_PROCPRIO packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_procprio(LMKD_CTRL_PACKET packet, struct lmk_procprio* params) {
    packet[0] = htonl(LMK_PROCPRIO);
    packet[1] = htonl(params->pid);
    packet[2] = htonl(params->uid);
    packet[3] = htonl(params->oomadj);
    packet[4] = htonl((int)params->ptype);
    return 5 * sizeof(int);
}

/* LMK_PROCREMOVE packet payload */
struct lmk_procremove {
    pid_t pid;
};

/*
 * For LMK_PROCREMOVE packet get its payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline void lmkd_pack_get_procremove(LMKD_CTRL_PACKET packet,
                                            struct lmk_procremove* params) {
    params->pid = (pid_t)ntohl(packet[1]);
}

/*
 * Prepare LMK_PROCREMOVE packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_procremove(LMKD_CTRL_PACKET packet,
                                              struct lmk_procremove* params) {
    packet[0] = htonl(LMK_PROCREMOVE);
    packet[1] = htonl(params->pid);
    return 2 * sizeof(int);
}

/*
 * Prepare LMK_PROCPURGE packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_procpurge(LMKD_CTRL_PACKET packet) {
    packet[0] = htonl(LMK_PROCPURGE);
    return sizeof(int);
}

/* LMK_GETKILLCNT packet payload */
struct lmk_getkillcnt {
    int min_oomadj;
    int max_oomadj;
};

/*
 * For LMK_GETKILLCNT packet get its payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline void lmkd_pack_get_getkillcnt(LMKD_CTRL_PACKET packet,
                                            struct lmk_getkillcnt* params) {
    params->min_oomadj = ntohl(packet[1]);
    params->max_oomadj = ntohl(packet[2]);
}

/*
 * Prepare LMK_GETKILLCNT packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_getkillcnt(LMKD_CTRL_PACKET packet,
                                              struct lmk_getkillcnt* params) {
    packet[0] = htonl(LMK_GETKILLCNT);
    packet[1] = htonl(params->min_oomadj);
    packet[2] = htonl(params->max_oomadj);
    return 3 * sizeof(int);
}

/*
 * Prepare LMK_GETKILLCNT reply packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_getkillcnt_repl(LMKD_CTRL_PACKET packet, int kill_cnt) {
    packet[0] = htonl(LMK_GETKILLCNT);
    packet[1] = htonl(kill_cnt);
    return 2 * sizeof(int);
}

/* Types of asynchronous events sent from lmkd to its clients */
enum async_event_type {
    LMK_ASYNC_EVENT_FIRST,
    LMK_ASYNC_EVENT_KILL = LMK_ASYNC_EVENT_FIRST,
    LMK_ASYNC_EVENT_STAT,
    LMK_ASYNC_EVENT_COUNT,
};

/* LMK_SUBSCRIBE packet payload */
struct lmk_subscribe {
    enum async_event_type evt_type;
};

/*
 * For LMK_SUBSCRIBE packet get its payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline void lmkd_pack_get_subscribe(LMKD_CTRL_PACKET packet, struct lmk_subscribe* params) {
    params->evt_type = (enum async_event_type)ntohl(packet[1]);
}

/**
 * Prepare LMK_SUBSCRIBE packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_subscribe(LMKD_CTRL_PACKET packet, enum async_event_type evt_type) {
    packet[0] = htonl(LMK_SUBSCRIBE);
    packet[1] = htonl((int)evt_type);
    return 2 * sizeof(int);
}

/**
 * Prepare LMK_PROCKILL unsolicited packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_prockills(LMKD_CTRL_PACKET packet, pid_t pid, uid_t uid) {
    packet[0] = htonl(LMK_PROCKILL);
    packet[1] = htonl(pid);
    packet[2] = htonl(uid);
    return 3 * sizeof(int);
}

/*
 * Prepare LMK_UPDATE_PROPS packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_update_props(LMKD_CTRL_PACKET packet) {
    packet[0] = htonl(LMK_UPDATE_PROPS);
    return sizeof(int);
}

/*
 * Prepare LMK_START_MONITORING packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_start_monitoring(LMKD_CTRL_PACKET packet) {
    packet[0] = htonl(LMK_START_MONITORING);
    return sizeof(int);
}

/*
 * Prepare LMK_UPDATE_PROPS reply packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_update_props_repl(LMKD_CTRL_PACKET packet, int result) {
    packet[0] = htonl(LMK_UPDATE_PROPS);
    packet[1] = htonl(result);
    return 2 * sizeof(int);
}

/* LMK_UPDATE_PROPS reply payload */
struct lmk_update_props_reply {
    int result;
};

/*
 * For LMK_UPDATE_PROPS reply payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline void lmkd_pack_get_update_props_repl(LMKD_CTRL_PACKET packet,
                                          struct lmk_update_props_reply* params) {
    params->result = ntohl(packet[1]);
}

/*
 * Prepare LMK_BOOT_COMPLETED packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_boot_completed_notif(LMKD_CTRL_PACKET packet) {
    packet[0] = htonl(LMK_BOOT_COMPLETED);
    return sizeof(int);
}

/*
 * Prepare LMK_BOOT_COMPLETED reply packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_boot_completed_notif_repl(LMKD_CTRL_PACKET packet, int result) {
    packet[0] = htonl(LMK_BOOT_COMPLETED);
    packet[1] = htonl(result);
    return 2 * sizeof(int);
}

/* LMK_BOOT_COMPLETED reply payload */
struct lmk_boot_completed_notif_reply {
    int result;
};

/*
 * For LMK_BOOT_COMPLETED reply payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline void lmkd_pack_get_boot_completed_notif_repl(
        LMKD_CTRL_PACKET packet, struct lmk_boot_completed_notif_reply* params) {
    params->result = ntohl(packet[1]);
}

#define PROCS_PRIO_MAX_RECORD_COUNT (CTRL_PACKET_MAX_SIZE / LMK_PROCPRIO_SIZE)

struct lmk_procs_prio {
    struct lmk_procprio procs[PROCS_PRIO_MAX_RECORD_COUNT];
};

/*
 * For LMK_PROCS_PRIO packet get its payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline int lmkd_pack_get_procs_prio(LMKD_CTRL_PACKET packet, struct lmk_procs_prio* params,
                                           const int field_count) {
    if (field_count < LMK_PROCPRIO_FIELD_COUNT || (field_count % LMK_PROCPRIO_FIELD_COUNT) != 0)
        return -1;
    const int procs_count = (field_count / LMK_PROCPRIO_FIELD_COUNT);

    /* Start packet at 1 since 0 is cmd type */
    int packetIdx = 1;
    for (int procs_idx = 0; procs_idx < procs_count; procs_idx++) {
        params->procs[procs_idx].pid = (pid_t)ntohl(packet[packetIdx++]);
        params->procs[procs_idx].uid = (uid_t)ntohl(packet[packetIdx++]);
        params->procs[procs_idx].oomadj = ntohl(packet[packetIdx++]);
        params->procs[procs_idx].ptype = (enum proc_type)ntohl(packet[packetIdx++]);
    }

    return procs_count;
}

/*
 * Prepare LMK_PROCS_PRIO packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_procs_prio(LMKD_CTRL_PACKET packet,
                                              struct lmk_procs_prio* params,
                                              const int procs_count) {
    packet[0] = htonl(LMK_PROCS_PRIO);
    int packetIdx = 1;

    for (int i = 0; i < procs_count; i++) {
        packet[packetIdx++] = htonl(params->procs[i].pid);
        packet[packetIdx++] = htonl(params->procs[i].uid);
        packet[packetIdx++] = htonl(params->procs[i].oomadj);
        packet[packetIdx++] = htonl((int)params->procs[i].ptype);
    }

    return packetIdx * sizeof(int);
}

__END_DECLS

#endif /* _LMKD_H_ */
