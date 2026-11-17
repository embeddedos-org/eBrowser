// SPDX-License-Identifier: MIT
#include "eBrowser/url.h"
#include "eBrowser/compat.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

int eb_url_default_port(const char *scheme) {
    if (!scheme) return 80;
    if (strcasecmp(scheme, "https") == 0) return 443;
    if (strcasecmp(scheme, "http") == 0) return 80;
    if (strcasecmp(scheme, "ftp") == 0) return 21;
    return 80;
}

bool eb_url_parse(const char *url_str, eb_url_t *out) {
    if (!url_str || !out) return false;
    memset(out, 0, sizeof(eb_url_t));

    const char *p = url_str;

    const char *scheme_end = strstr(p, "://");
    if (scheme_end) {
        size_t slen = (size_t)(scheme_end - p);
        if (slen >= EB_URL_MAX_SCHEME) slen = EB_URL_MAX_SCHEME - 1;
        for (size_t i = 0; i < slen; i++)
            out->scheme[i] = (char)tolower((unsigned char)p[i]);
        p = scheme_end + 3;
    } else {
        strncpy(out->scheme, "http", EB_URL_MAX_SCHEME - 1);
    }

    out->port = eb_url_default_port(out->scheme);

    const char *host_end = p;
    while (*host_end && *host_end != '/' && *host_end != ':' && *host_end != '?' && *host_end != '#')
        host_end++;

    size_t hlen = (size_t)(host_end - p);
    if (hlen >= EB_URL_MAX_HOST) hlen = EB_URL_MAX_HOST - 1;
    strncpy(out->host, p, hlen);
    p = host_end;

    if (*p == ':') {
        p++;
        out->port = atoi(p);
        while (*p && isdigit((unsigned char)*p)) p++;
    }

    if (*p == '/') {
        const char *path_end = p;
        while (*path_end && *path_end != '?' && *path_end != '#') path_end++;
        size_t plen = (size_t)(path_end - p);
        if (plen >= EB_URL_MAX_PATH) plen = EB_URL_MAX_PATH - 1;
        strncpy(out->path, p, plen);
        p = path_end;
    } else {
        strncpy(out->path, "/", EB_URL_MAX_PATH - 1);
    }

    if (*p == '?') {
        p++;
        const char *qend = p;
        while (*qend && *qend != '#') qend++;
        size_t qlen = (size_t)(qend - p);
        if (qlen >= EB_URL_MAX_QUERY) qlen = EB_URL_MAX_QUERY - 1;
        strncpy(out->query, p, qlen);
        p = qend;
    }

    if (*p == '#') {
        p++;
        strncpy(out->fragment, p, EB_URL_MAX_FRAGMENT - 1);
    }

    return out->host[0] != '\0';
}

void eb_url_to_string(const eb_url_t *url, char *buf, size_t buf_len) {
    if (!url || !buf || buf_len == 0) return;
    int port_default = eb_url_default_port(url->scheme);
    if (url->port != port_default && url->port > 0) {
        snprintf(buf, buf_len, "%s://%s:%d%s%s%s%s%s",
                 url->scheme, url->host, url->port, url->path,
                 url->query[0] ? "?" : "", url->query,
                 url->fragment[0] ? "#" : "", url->fragment);
    } else {
        snprintf(buf, buf_len, "%s://%s%s%s%s%s%s",
                 url->scheme, url->host, url->path,
                 url->query[0] ? "?" : "", url->query,
                 url->fragment[0] ? "#" : "", url->fragment);
    }
}

/* Copy n bytes of src into dst (capacity cap), truncating, always terminated. */
static void url_copy_span(char *dst, size_t cap, const char *src, size_t n) {
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/* RFC 3986 section 5.2.4: drop "." segments and let ".." remove the segment
 * before it, in place. path starts with '/'; the result is never longer. */
static void url_remove_dot_segments(char *path) {
    char out[EB_URL_MAX_PATH];
    size_t olen = 0;
    const char *p = path;

    while (*p == '/') {
        const char *seg = p + 1;
        const char *end = seg;
        while (*end && *end != '/') end++;
        size_t slen = (size_t)(end - seg);

        if (slen == 1 && seg[0] == '.') {
            if (!*end) out[olen++] = '/';
        } else if (slen == 2 && seg[0] == '.' && seg[1] == '.') {
            while (olen > 0 && out[olen - 1] != '/') olen--;
            if (olen > 0) olen--;
            if (!*end) out[olen++] = '/';
        } else if (olen + 1 + slen < sizeof(out)) {
            out[olen++] = '/';
            memcpy(out + olen, seg, slen);
            olen += slen;
        }
        p = end;
    }
    if (olen == 0) out[olen++] = '/';
    out[olen] = '\0';
    memcpy(path, out, olen + 1);
}

/* Resolve a reference against base as RFC 3986 section 5.2.2 does. */
bool eb_url_resolve(const eb_url_t *base, const char *relative, eb_url_t *out) {
    if (!base || !relative || !out) return false;

    if (strstr(relative, "://")) return eb_url_parse(relative, out);

    /* "//host/path": a new host, same scheme. */
    if (relative[0] == '/' && relative[1] == '/') {
        char full[EB_URL_MAX_FULL];
        snprintf(full, sizeof(full), "%s:%s", base->scheme, relative);
        return eb_url_parse(full, out);
    }

    size_t path_len = strcspn(relative, "?#");
    const char *query = relative[path_len] == '?' ? relative + path_len + 1 : NULL;
    const char *hash = strchr(relative + path_len, '#');

    *out = *base;
    out->fragment[0] = '\0';

    if (path_len > 0 || query) {
        out->query[0] = '\0';
        if (query)
            url_copy_span(out->query, EB_URL_MAX_QUERY, query,
                          hash ? (size_t)(hash - query) : strlen(query));
    }

    if (path_len > 0) {
        if (relative[0] == '/') {
            url_copy_span(out->path, EB_URL_MAX_PATH, relative, path_len);
        } else {
            /* Merge: the base path up to its last '/', then the reference. */
            char *last_slash = strrchr(out->path, '/');
            if (last_slash) *(last_slash + 1) = '\0';
            else url_copy_span(out->path, EB_URL_MAX_PATH, "/", 1);
            size_t plen = strlen(out->path);
            url_copy_span(out->path + plen, EB_URL_MAX_PATH - plen, relative, path_len);
        }
        url_remove_dot_segments(out->path);
    }

    if (hash) url_copy_span(out->fragment, EB_URL_MAX_FRAGMENT, hash + 1, strlen(hash + 1));
    return true;
}
