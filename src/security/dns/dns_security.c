// SPDX-License-Identifier: MIT
#include "eBrowser/dns_security.h"
#include "eBrowser/compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

void eb_dns_init(eb_dns_security_t *d) {
    if (!d) return; memset(d, 0, sizeof(*d));
    d->mode = EB_DNS_DOH;
    d->cache_enabled = true;
    d->rebind_protection = true;
    /* Add default DoH servers */
    eb_dns_add_server(d, "https://cloudflare-dns.com/dns-query");
    eb_dns_add_server(d, "https://dns.google/dns-query");
}

void eb_dns_destroy(eb_dns_security_t *d) { if (d) memset(d, 0, sizeof(*d)); }

void eb_dns_set_mode(eb_dns_security_t *d, eb_dns_mode_t m) { if (d) d->mode = m; }

bool eb_dns_add_server(eb_dns_security_t *d, const char *url) {
    if (!d || !url || d->server_count >= EB_DNS_MAX_SERVERS) return false;
    eb_dns_server_t *s = &d->servers[d->server_count];
    strncpy(s->doh_url, url, sizeof(s->doh_url)-1);
    s->enabled = true; d->server_count++;
    return true;
}

bool eb_dns_cache_lookup(eb_dns_security_t *d, const char *name,
                          eb_dns_type_t type, eb_dns_record_t *r) {
    if (!d || !name || !r || !d->cache_enabled) return false;
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t now = (uint64_t)ts.tv_sec;
    for (int i = 0; i < d->cache_count; i++) {
        eb_dns_record_t *c = &d->cache[i];
        if (strcasecmp(c->name, name) == 0 && (now - c->cached_at) < c->ttl) {
            *r = *c; d->cache_hits++; return true;
        }
    }
    d->cache_misses++;
    return false;
}

void eb_dns_cache_insert(eb_dns_security_t *d, const eb_dns_record_t *r) {
    if (!d || !r || !d->cache_enabled) return;
    /* Evict oldest if full */
    int idx = d->cache_count;
    if (idx >= EB_DNS_MAX_CACHE) {
        uint64_t oldest = ~0ULL; idx = 0;
        for (int i = 0; i < EB_DNS_MAX_CACHE; i++)
            if (d->cache[i].cached_at < oldest) { oldest = d->cache[i].cached_at; idx = i; }
    } else { d->cache_count++; }
    d->cache[idx] = *r;
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    d->cache[idx].cached_at = (uint64_t)ts.tv_sec;
}

void eb_dns_cache_flush(eb_dns_security_t *d) {
    if (!d) return;
    memset(d->cache, 0, sizeof(d->cache)); d->cache_count = 0;
}

bool eb_dns_is_rebind_attack(const eb_dns_record_t *r) {
    if (!r) return false;
    /* Check if any resolved address is a private/loopback IP */
    for (int i = 0; i < r->addr_count; i++) {
        const char *a = r->addrs[i];
        if (strncmp(a, "10.", 3) == 0 ||
            strncmp(a, "127.", 4) == 0 ||
            strncmp(a, "192.168.", 8) == 0 ||
            strncmp(a, "172.16.", 7) == 0 ||
            strncmp(a, "172.17.", 7) == 0 ||
            strncmp(a, "172.18.", 7) == 0 ||
            strncmp(a, "172.19.", 7) == 0 ||
            strncmp(a, "0.", 2) == 0 ||
            strcmp(a, "::1") == 0 ||
            strncmp(a, "fe80:", 5) == 0 ||
            strncmp(a, "fd", 2) == 0)
            return true;
    }
    return false;
}

int eb_dns_resolve_doh(eb_dns_security_t *d, const char *name,
                        eb_dns_type_t type, eb_dns_record_t *r) {
    if (!d || !name || !r) return -1;
    /* Check cache first */
    if (eb_dns_cache_lookup(d, name, type, r)) return 0;
    d->queries++;
    /* In production: send DNS wireformat query over HTTPS POST to DoH server.
       For now, fall through to system resolver as fallback. */
    return eb_dns_resolve(d, name, type, r);
}

int eb_dns_resolve(eb_dns_security_t *d, const char *name,
                    eb_dns_type_t type, eb_dns_record_t *r) {
    if (!d || !name || !r) return -1;
    if (eb_dns_cache_lookup(d, name, type, r)) return 0;
    d->queries++;
    /* System resolver fallback - in production this would use
       getaddrinfo() and then cache the result */
    memset(r, 0, sizeof(*r));
    strncpy(r->name, name, EB_DNS_MAX_NAME-1);
    r->ttl = 300; /* 5 min default */
    /* Check for rebinding if enabled */
    if (d->rebind_protection && eb_dns_is_rebind_attack(r)) {
        d->dnssec_failures++;
        return -2; /* Rebind attack detected */
    }
    eb_dns_cache_insert(d, r);
    return 0;
}
