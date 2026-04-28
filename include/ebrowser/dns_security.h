// SPDX-License-Identifier: MIT
#ifndef eBrowser_DNS_SECURITY_H
#define eBrowser_DNS_SECURITY_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define EB_DNS_MAX_CACHE   512
#define EB_DNS_MAX_SERVERS 8
#define EB_DNS_MAX_NAME    256
#define EB_DNS_MAX_ADDRS   16

typedef enum { EB_DNS_PLAIN, EB_DNS_DOH, EB_DNS_DOT } eb_dns_mode_t;
typedef enum { EB_DNS_A=1, EB_DNS_AAAA=28, EB_DNS_CNAME=5, EB_DNS_HTTPS=65 } eb_dns_type_t;

typedef struct {
    char name[EB_DNS_MAX_NAME]; char addrs[EB_DNS_MAX_ADDRS][64]; int addr_count;
    uint32_t ttl; uint64_t cached_at; bool dnssec_valid, is_cname; char cname[EB_DNS_MAX_NAME];
} eb_dns_record_t;

typedef struct { char doh_url[512]; char dot_host[256]; uint16_t dot_port; bool enabled; } eb_dns_server_t;

typedef struct {
    eb_dns_mode_t mode; eb_dns_server_t servers[EB_DNS_MAX_SERVERS]; int server_count;
    eb_dns_record_t cache[EB_DNS_MAX_CACHE]; int cache_count;
    bool dnssec_required, rebind_protection, cache_enabled;
    uint64_t queries, cache_hits, cache_misses, dnssec_failures;
} eb_dns_security_t;

void  eb_dns_init(eb_dns_security_t *dns);
void  eb_dns_destroy(eb_dns_security_t *dns);
void  eb_dns_set_mode(eb_dns_security_t *dns, eb_dns_mode_t mode);
bool  eb_dns_add_server(eb_dns_security_t *dns, const char *url);
int   eb_dns_resolve(eb_dns_security_t *dns, const char *name, eb_dns_type_t type, eb_dns_record_t *result);
int   eb_dns_resolve_doh(eb_dns_security_t *dns, const char *name, eb_dns_type_t type, eb_dns_record_t *result);
bool  eb_dns_cache_lookup(eb_dns_security_t *dns, const char *name, eb_dns_type_t type, eb_dns_record_t *result);
void  eb_dns_cache_insert(eb_dns_security_t *dns, const eb_dns_record_t *record);
void  eb_dns_cache_flush(eb_dns_security_t *dns);
bool  eb_dns_is_rebind_attack(const eb_dns_record_t *record);
#endif
