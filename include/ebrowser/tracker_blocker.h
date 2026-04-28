// SPDX-License-Identifier: MIT
#ifndef eBrowser_TRACKER_BLOCKER_H
#define eBrowser_TRACKER_BLOCKER_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define EB_TB_MAX_FILTERS   16384
#define EB_TB_MAX_PATTERN   512
#define EB_TB_MAX_DOMAIN    256
#define EB_TB_MAX_LISTS     32
#define EB_TB_MAX_COSMETIC  4096

typedef enum { EB_TB_BLOCK, EB_TB_ALLOW, EB_TB_REDIRECT, EB_TB_CSP_INJECT } eb_tb_action_t;
typedef enum {
    EB_TB_RES_SCRIPT=(1<<0), EB_TB_RES_IMAGE=(1<<1), EB_TB_RES_STYLESHEET=(1<<2),
    EB_TB_RES_FONT=(1<<3), EB_TB_RES_XHR=(1<<4), EB_TB_RES_MEDIA=(1<<5),
    EB_TB_RES_WEBSOCKET=(1<<6), EB_TB_RES_DOCUMENT=(1<<7), EB_TB_RES_POPUP=(1<<9),
    EB_TB_RES_ALL=0x3FF
} eb_tb_resource_t;

typedef struct {
    char pattern[EB_TB_MAX_PATTERN]; eb_tb_action_t action; uint32_t resource_types;
    char domain_include[EB_TB_MAX_DOMAIN], domain_exclude[EB_TB_MAX_DOMAIN];
    bool is_regex, match_case, third_party_only, first_party_only, enabled; uint64_t hits;
} eb_tb_filter_t;

typedef struct { char selector[EB_TB_MAX_PATTERN]; char domain[EB_TB_MAX_DOMAIN]; bool is_exception; } eb_tb_cosmetic_t;
typedef struct { char name[128]; char url[512]; bool enabled; int filter_count; uint64_t last_updated; } eb_tb_list_t;

typedef struct {
    eb_tb_filter_t filters[EB_TB_MAX_FILTERS]; int filter_count;
    eb_tb_cosmetic_t cosmetics[EB_TB_MAX_COSMETIC]; int cosmetic_count;
    eb_tb_list_t lists[EB_TB_MAX_LISTS]; int list_count;
    bool enabled, block_third_party_cookies, strip_referrer, block_cname_cloaking, first_party_isolation;
    uint64_t total_blocked, total_allowed, total_cosmetic;
} eb_tracker_blocker_t;

void  eb_tb_init(eb_tracker_blocker_t *tb);
void  eb_tb_destroy(eb_tracker_blocker_t *tb);
int   eb_tb_load_filterlist(eb_tracker_blocker_t *tb, const char *path);
int   eb_tb_parse_filter_line(eb_tracker_blocker_t *tb, const char *line);
bool  eb_tb_should_block(eb_tracker_blocker_t *tb, const char *url, const char *page_domain, eb_tb_resource_t type);
bool  eb_tb_add_custom_filter(eb_tracker_blocker_t *tb, const char *filter);
bool  eb_tb_remove_filter(eb_tracker_blocker_t *tb, const char *pattern);
typedef struct { uint64_t blocked, allowed, trackers, ads, scripts, cookies, cosmetic; int filters, lists; } eb_tb_stats_t;
void  eb_tb_get_stats(const eb_tracker_blocker_t *tb, eb_tb_stats_t *stats);
#endif
