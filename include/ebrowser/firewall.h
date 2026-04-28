// SPDX-License-Identifier: MIT
#ifndef eBrowser_FIREWALL_H
#define eBrowser_FIREWALL_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define EB_FW_MAX_RULES     512
#define EB_FW_MAX_DOMAIN    256
#define EB_FW_MAX_RATE      128
#define EB_FW_MAX_BLOCKLIST 2048

typedef enum { EB_FW_ALLOW, EB_FW_BLOCK, EB_FW_LOG, EB_FW_THROTTLE, EB_FW_UPGRADE_HTTPS } eb_fw_action_t;
typedef enum { EB_FW_EXACT, EB_FW_WILDCARD, EB_FW_CIDR, EB_FW_PORT_RANGE } eb_fw_match_t;
typedef enum { EB_FW_PROTO_ANY, EB_FW_PROTO_HTTP, EB_FW_PROTO_HTTPS, EB_FW_PROTO_WS, EB_FW_PROTO_WSS } eb_fw_proto_t;

typedef struct {
    eb_fw_action_t action; eb_fw_match_t match_type; eb_fw_proto_t protocol;
    char pattern[EB_FW_MAX_DOMAIN]; uint16_t port_min, port_max;
    int priority; bool enabled; uint64_t hits;
} eb_fw_rule_t;

typedef struct {
    char domain[EB_FW_MAX_DOMAIN]; int max_per_sec, max_per_min, cur_sec, cur_min;
    uint64_t sec_start, min_start; bool throttled;
} eb_fw_rate_t;

typedef struct {
    eb_fw_rule_t rules[EB_FW_MAX_RULES]; int rule_count;
    eb_fw_rate_t rates[EB_FW_MAX_RATE]; int rate_count;
    char blocklist[EB_FW_MAX_BLOCKLIST][EB_FW_MAX_DOMAIN]; int bl_count;
    bool force_https, block_mixed, block_nonstandard_ports, enabled;
    uint64_t total_blocked, total_allowed;
} eb_firewall_t;

void           eb_fw_init(eb_firewall_t *fw);
void           eb_fw_destroy(eb_firewall_t *fw);
bool           eb_fw_add_rule(eb_firewall_t *fw, const eb_fw_rule_t *rule);
bool           eb_fw_block_domain(eb_firewall_t *fw, const char *domain);
bool           eb_fw_unblock_domain(eb_firewall_t *fw, const char *domain);
bool           eb_fw_is_blocked(const eb_firewall_t *fw, const char *domain);
int            eb_fw_load_blocklist(eb_firewall_t *fw, const char *path);
eb_fw_action_t eb_fw_check(eb_firewall_t *fw, const char *url, const char *domain, uint16_t port, eb_fw_proto_t proto);
bool           eb_fw_add_rate_limit(eb_firewall_t *fw, const char *domain, int per_sec, int per_min);
bool           eb_fw_check_rate(eb_firewall_t *fw, const char *domain);
bool           eb_fw_wildcard_match(const char *pattern, const char *domain);
bool           eb_fw_cidr_match(const char *cidr, uint32_t ip);
typedef struct { uint64_t total_requests, blocked, allowed, throttled, https_upgrades; } eb_fw_stats_t;
void           eb_fw_get_stats(const eb_firewall_t *fw, eb_fw_stats_t *stats);
#endif
