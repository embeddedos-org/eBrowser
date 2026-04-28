// SPDX-License-Identifier: MIT
#include "eBrowser/firewall.h"
#include "eBrowser/compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

static uint64_t fw_now(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

void eb_fw_init(eb_firewall_t *fw) {
    if (!fw) return; memset(fw, 0, sizeof(*fw));
    fw->enabled = true; fw->force_https = true;
    fw->block_mixed = true; fw->block_nonstandard_ports = true;
}
void eb_fw_destroy(eb_firewall_t *fw) { if (fw) memset(fw, 0, sizeof(*fw)); }

bool eb_fw_add_rule(eb_firewall_t *fw, const eb_fw_rule_t *r) {
    if (!fw || !r || fw->rule_count >= EB_FW_MAX_RULES) return false;
    fw->rules[fw->rule_count] = *r; fw->rules[fw->rule_count].enabled = true;
    fw->rule_count++; return true;
}

bool eb_fw_block_domain(eb_firewall_t *fw, const char *d) {
    if (!fw || !d || fw->bl_count >= EB_FW_MAX_BLOCKLIST) return false;
    for (int i = 0; i < fw->bl_count; i++) if (strcasecmp(fw->blocklist[i], d) == 0) return true;
    strncpy(fw->blocklist[fw->bl_count++], d, EB_FW_MAX_DOMAIN - 1);
    return true;
}

bool eb_fw_unblock_domain(eb_firewall_t *fw, const char *d) {
    if (!fw || !d) return false;
    for (int i = 0; i < fw->bl_count; i++)
        if (strcasecmp(fw->blocklist[i], d) == 0) {
            for (int j = i; j < fw->bl_count - 1; j++) strcpy(fw->blocklist[j], fw->blocklist[j+1]);
            fw->bl_count--; return true;
        }
    return false;
}

bool eb_fw_is_blocked(const eb_firewall_t *fw, const char *d) {
    if (!fw || !d) return false;
    for (int i = 0; i < fw->bl_count; i++) {
        if (strcasecmp(fw->blocklist[i], d) == 0) return true;
        if (fw->blocklist[i][0] == '*' && fw->blocklist[i][1] == '.') {
            const char *suf = fw->blocklist[i] + 1;
            size_t sl = strlen(suf), dl = strlen(d);
            if (dl >= sl && strcasecmp(d + dl - sl, suf) == 0) return true;
        }
    }
    return false;
}

int eb_fw_load_blocklist(eb_firewall_t *fw, const char *path) {
    if (!fw || !path) return -1;
    FILE *f = fopen(path, "r"); if (!f) return -1;
    char line[EB_FW_MAX_DOMAIN]; int n = 0;
    while (fgets(line, sizeof(line), f)) {
        char *nl = strchr(line, '\n'); if (nl) *nl = '\0';
        char *c = strchr(line, '#'); if (c) *c = '\0';
        char *s = line; while (*s && isspace((unsigned char)*s)) s++;
        if (!*s) continue;
        char *e = s + strlen(s) - 1; while (e > s && isspace((unsigned char)*e)) *e-- = '\0';
        if (eb_fw_block_domain(fw, s)) n++;
    }
    fclose(f); return n;
}

bool eb_fw_wildcard_match(const char *pat, const char *dom) {
    if (!pat || !dom) return false;
    if (strcmp(pat, "*") == 0) return true;
    const char *p = pat, *d = dom;
    while (*p && *d) {
        if (*p == '*') { p++; if (*p == '.') p++;
            const char *dot = strchr(d, '.'); if (dot) d = dot + 1; else return !*p;
        } else { if (tolower((unsigned char)*p) != tolower((unsigned char)*d)) return false; p++; d++; }
    }
    return !*p && !*d;
}

bool eb_fw_cidr_match(const char *cidr, uint32_t ip) {
    if (!cidr) return false;
    unsigned a,b,c,d,pfx;
    if (sscanf(cidr, "%u.%u.%u.%u/%u", &a, &b, &c, &d, &pfx) != 5 || pfx > 32) return false;
    uint32_t net = (a<<24)|(b<<16)|(c<<8)|d;
    uint32_t mask = pfx ? (~0u << (32-pfx)) : 0;
    return (ip & mask) == (net & mask);
}

eb_fw_action_t eb_fw_check(eb_firewall_t *fw, const char *url,
                            const char *dom, uint16_t port, eb_fw_proto_t proto) {
    if (!fw || !fw->enabled) return EB_FW_ALLOW;
    if (dom && eb_fw_is_blocked(fw, dom)) { fw->total_blocked++; return EB_FW_BLOCK; }
    if (fw->force_https && proto == EB_FW_PROTO_HTTP) { fw->total_blocked++; return EB_FW_UPGRADE_HTTPS; }
    if (fw->block_nonstandard_ports && port != 80 && port != 443 && port != 8080 && port != 8443 && port != 0) { fw->total_blocked++; return EB_FW_BLOCK; }
    if (dom && !eb_fw_check_rate(fw, dom)) { fw->total_blocked++; return EB_FW_THROTTLE; }
    for (int i = 0; i < fw->rule_count; i++) {
        eb_fw_rule_t *r = &fw->rules[i];
        if (!r->enabled) continue;
        if (r->protocol != EB_FW_PROTO_ANY && r->protocol != proto) continue;
        bool m = false;
        if (r->match_type == EB_FW_EXACT) m = dom && strcasecmp(r->pattern, dom) == 0;
        else if (r->match_type == EB_FW_WILDCARD) m = dom && eb_fw_wildcard_match(r->pattern, dom);
        else if (r->match_type == EB_FW_PORT_RANGE) m = port >= r->port_min && port <= r->port_max;
        if (m) { r->hits++; if (r->action == EB_FW_BLOCK) fw->total_blocked++; else fw->total_allowed++; return r->action; }
    }
    fw->total_allowed++; return EB_FW_ALLOW;
}

bool eb_fw_add_rate_limit(eb_firewall_t *fw, const char *dom, int ps, int pm) {
    if (!fw || !dom || fw->rate_count >= EB_FW_MAX_RATE) return false;
    eb_fw_rate_t *r = &fw->rates[fw->rate_count++];
    strncpy(r->domain, dom, EB_FW_MAX_DOMAIN-1);
    r->max_per_sec = ps; r->max_per_min = pm;
    r->sec_start = r->min_start = fw_now();
    return true;
}

bool eb_fw_check_rate(eb_firewall_t *fw, const char *dom) {
    if (!fw || !dom) return true;
    uint64_t now = fw_now();
    for (int i = 0; i < fw->rate_count; i++) {
        eb_fw_rate_t *r = &fw->rates[i];
        if (strcasecmp(r->domain, dom) != 0) continue;
        if (now - r->sec_start >= 1000) { r->cur_sec = 0; r->sec_start = now; }
        if (now - r->min_start >= 60000) { r->cur_min = 0; r->min_start = now; }
        r->cur_sec++; r->cur_min++;
        if (r->cur_sec > r->max_per_sec || r->cur_min > r->max_per_min) { r->throttled = true; return false; }
        r->throttled = false;
    }
    return true;
}

void eb_fw_get_stats(const eb_firewall_t *fw, eb_fw_stats_t *s) {
    if (!fw || !s) return;
    s->total_requests = fw->total_blocked + fw->total_allowed;
    s->blocked = fw->total_blocked; s->allowed = fw->total_allowed;
    s->throttled = s->https_upgrades = 0;
}
