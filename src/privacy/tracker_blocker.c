// SPDX-License-Identifier: MIT
#include "eBrowser/tracker_blocker.h"
#include "eBrowser/compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>

void eb_tb_init(eb_tracker_blocker_t *tb) {
    if (!tb) return; memset(tb, 0, sizeof(*tb));
    tb->enabled = true;
    tb->block_third_party_cookies = true;
    tb->strip_referrer = true;
    tb->block_cname_cloaking = true;
    tb->first_party_isolation = true;
}

void eb_tb_destroy(eb_tracker_blocker_t *tb) { if (tb) memset(tb, 0, sizeof(*tb)); }

static bool pattern_match(const char *pat, const char *url) {
    if (!pat || !url) return false;
    /* Simple wildcard matching for filter patterns:
       ||domain^ matches domain and subdomains
       *pattern* matches anywhere in URL
       |url matches start of URL */
    if (pat[0] == '|' && pat[1] == '|') {
        /* Domain anchor: ||example.com^ */
        const char *dom = pat + 2;
        const char *end = strchr(dom, '^');
        size_t dlen = end ? (size_t)(end - dom) : strlen(dom);
        const char *host = strstr(url, "://");
        if (host) host += 3; else host = url;
        const char *hend = strchr(host, '/');
        size_t hlen = hend ? (size_t)(hend - host) : strlen(host);
        /* Check if host ends with domain */
        if (hlen >= dlen) {
            const char *suffix = host + hlen - dlen;
            if (strncasecmp(suffix, dom, dlen) == 0) {
                if (suffix == host || *(suffix-1) == '.') return true;
            }
        }
        return false;
    }
    if (pat[0] == '|') {
        return strncasecmp(url, pat+1, strlen(pat+1)) == 0;
    }
    /* Simple substring/wildcard match */
    const char *p = pat, *u = url;
    while (*p && *u) {
        if (*p == '*') { p++;
            if (!*p) return true;
            const char *f = strcasestr(u, p);
            if (!f) return false;
            u = f; continue;
        }
        if (tolower((unsigned char)*p) != tolower((unsigned char)*u)) return false;
        p++; u++;
    }
    return *p == '\0' || (*p == '*' && *(p+1) == '\0');
}

int eb_tb_parse_filter_line(eb_tracker_blocker_t *tb, const char *line) {
    if (!tb || !line) return -1;
    if (tb->filter_count >= EB_TB_MAX_FILTERS) return -1;
    /* Skip comments and empty lines */
    while (*line && isspace((unsigned char)*line)) line++;
    if (!*line || *line == '!' || *line == '[') return 0;

    eb_tb_filter_t *f = &tb->filters[tb->filter_count];
    memset(f, 0, sizeof(*f));
    f->action = EB_TB_BLOCK;
    f->resource_types = EB_TB_RES_ALL;
    f->enabled = true;

    /* Exception filter: @@pattern */
    if (line[0] == '@' && line[1] == '@') {
        f->action = EB_TB_ALLOW;
        line += 2;
    }

    /* Cosmetic filter: domain##selector */
    const char *css = strstr(line, "##");
    if (css && tb->cosmetic_count < EB_TB_MAX_COSMETIC) {
        eb_tb_cosmetic_t *c = &tb->cosmetics[tb->cosmetic_count];
        memset(c, 0, sizeof(*c));
        if (css > line) {
            size_t dlen = (size_t)(css - line);
            if (dlen < EB_TB_MAX_DOMAIN) memcpy(c->domain, line, dlen);
        }
        strncpy(c->selector, css + 2, EB_TB_MAX_PATTERN - 1);
        tb->cosmetic_count++;
        return 1;
    }

    /* Parse options after $ */
    const char *opts = strchr(line, '$');
    size_t plen = opts ? (size_t)(opts - line) : strlen(line);
    if (plen >= EB_TB_MAX_PATTERN) plen = EB_TB_MAX_PATTERN - 1;
    memcpy(f->pattern, line, plen);

    if (opts) {
        opts++;
        if (strstr(opts, "third-party")) f->third_party_only = true;
        if (strstr(opts, "first-party")) f->first_party_only = true;
        if (strstr(opts, "script")) f->resource_types = EB_TB_RES_SCRIPT;
        if (strstr(opts, "image")) f->resource_types = EB_TB_RES_IMAGE;
        if (strstr(opts, "stylesheet")) f->resource_types = EB_TB_RES_STYLESHEET;
        if (strstr(opts, "xmlhttprequest")) f->resource_types = EB_TB_RES_XHR;
        if (strstr(opts, "popup")) f->resource_types = EB_TB_RES_POPUP;
        /* Domain filter: domain=example.com|~other.com */
        const char *dom = strstr(opts, "domain=");
        if (dom) {
            dom += 7;
            const char *end = strchr(dom, ',');
            size_t dl = end ? (size_t)(end-dom) : strlen(dom);
            if (dom[0] == '~') {
                if (dl-1 < EB_TB_MAX_DOMAIN) memcpy(f->domain_exclude, dom+1, dl-1);
            } else {
                if (dl < EB_TB_MAX_DOMAIN) memcpy(f->domain_include, dom, dl);
            }
        }
    }

    tb->filter_count++;
    return 1;
}

int eb_tb_load_filterlist(eb_tracker_blocker_t *tb, const char *path) {
    if (!tb || !path) return -1;
    FILE *fp = fopen(path, "r"); if (!fp) return -1;
    char line[1024]; int n = 0;
    while (fgets(line, sizeof(line), fp)) {
        char *nl = strchr(line, '\n'); if (nl) *nl = '\0';
        if (eb_tb_parse_filter_line(tb, line) > 0) n++;
    }
    fclose(fp); return n;
}

bool eb_tb_should_block(eb_tracker_blocker_t *tb, const char *url,
                         const char *page, eb_tb_resource_t type) {
    if (!tb || !tb->enabled || !url) return false;

    /* Check exception filters first */
    for (int i = 0; i < tb->filter_count; i++) {
        eb_tb_filter_t *f = &tb->filters[i];
        if (!f->enabled || f->action != EB_TB_ALLOW) continue;
        if (!(f->resource_types & type)) continue;
        if (pattern_match(f->pattern, url)) {
            tb->total_allowed++; return false;
        }
    }

    /* Check block filters */
    for (int i = 0; i < tb->filter_count; i++) {
        eb_tb_filter_t *f = &tb->filters[i];
        if (!f->enabled || f->action != EB_TB_BLOCK) continue;
        if (!(f->resource_types & type)) continue;
        /* Domain restriction */
        if (f->domain_include[0] && page) {
            if (!strcasestr(page, f->domain_include)) continue;
        }
        if (f->domain_exclude[0] && page) {
            if (strcasestr(page, f->domain_exclude)) continue;
        }
        if (pattern_match(f->pattern, url)) {
            f->hits++; tb->total_blocked++;
            return true;
        }
    }
    tb->total_allowed++;
    return false;
}

bool eb_tb_add_custom_filter(eb_tracker_blocker_t *tb, const char *filter) {
    return eb_tb_parse_filter_line(tb, filter) > 0;
}

bool eb_tb_remove_filter(eb_tracker_blocker_t *tb, const char *pattern) {
    if (!tb || !pattern) return false;
    for (int i = 0; i < tb->filter_count; i++) {
        if (strcmp(tb->filters[i].pattern, pattern) == 0) {
            for (int j = i; j < tb->filter_count-1; j++)
                tb->filters[j] = tb->filters[j+1];
            tb->filter_count--; return true;
        }
    }
    return false;
}

void eb_tb_get_stats(const eb_tracker_blocker_t *tb, eb_tb_stats_t *s) {
    if (!tb || !s) return; memset(s, 0, sizeof(*s));
    s->blocked = tb->total_blocked; s->allowed = tb->total_allowed;
    s->cosmetic = tb->total_cosmetic; s->filters = tb->filter_count;
    s->lists = tb->list_count;
    for (int i = 0; i < tb->filter_count; i++) {
        if (tb->filters[i].resource_types & EB_TB_RES_SCRIPT) s->scripts += tb->filters[i].hits;
    }
    s->trackers = s->blocked; s->ads = s->blocked / 2; /* Estimate */
}
