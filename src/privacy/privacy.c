// SPDX-License-Identifier: MIT
#include "eBrowser/privacy.h"
#include "eBrowser/compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>

static const char *s_default_tracking[] = {
    "utm_source","utm_medium","utm_campaign","utm_term","utm_content",
    "fbclid","gclid","gclsrc","dclid","msclkid","mc_cid","mc_eid",
    "yclid","_openstat","twclid","igshid","s_cid","ref","ref_src",
    "ncid","ocid","zanpid","otc","oly_anon_id","oly_enc_id",NULL
};

void eb_priv_init(eb_privacy_t *p) {
    if (!p) return; memset(p, 0, sizeof(*p));
    p->mode = EB_PRIV_NORMAL;
    p->cookie_policy = EB_COOKIE_BLOCK_THIRD_PARTY;
    p->referrer_policy = EB_REF_ORIGIN_ONLY;
    p->dnt_enabled = true; p->gpc_enabled = true;
    p->strip_tracking_params = true;
    p->block_bounce_tracking = true;
    p->https_only = true;
    for (int i = 0; s_default_tracking[i]; i++)
        eb_priv_add_tracking_param(p, s_default_tracking[i]);
}

void eb_priv_destroy(eb_privacy_t *p) { if (p) memset(p, 0, sizeof(*p)); }

void eb_priv_set_mode(eb_privacy_t *p, eb_priv_mode_t m) {
    if (!p) return; p->mode = m;
    switch (m) {
    case EB_PRIV_INCOGNITO:
        p->cookie_policy = EB_COOKIE_DELETE_ON_CLOSE;
        p->referrer_policy = EB_REF_ORIGIN_ONLY;
        p->clear_on_exit = true; break;
    case EB_PRIV_TOR_MODE:
        p->cookie_policy = EB_COOKIE_BLOCK_ALL;
        p->referrer_policy = EB_REF_NONE;
        p->fpi_enabled = true; p->storage_partition = true;
        p->clear_on_exit = true; break;
    default: break;
    }
}

bool eb_priv_is_tracking_param(const eb_privacy_t *p, const char *param) {
    if (!p || !param) return false;
    for (int i = 0; i < p->tracking_param_count; i++)
        if (strcasecmp(p->tracking_params[i], param) == 0) return true;
    return false;
}

void eb_priv_add_tracking_param(eb_privacy_t *p, const char *param) {
    if (!p || !param || p->tracking_param_count >= EB_PRIV_MAX_TRACKING_PARAMS) return;
    strncpy(p->tracking_params[p->tracking_param_count++], param, EB_PRIV_MAX_PARAM_LEN-1);
}

int eb_priv_clean_url(eb_privacy_t *p, const char *url, char *clean, size_t max) {
    if (!p || !url || !clean || !p->strip_tracking_params) {
        if (clean && url) strncpy(clean, url, max-1);
        return 0;
    }
    const char *q = strchr(url, '?');
    if (!q) { strncpy(clean, url, max-1); return 0; }
    size_t base_len = (size_t)(q - url);
    if (base_len >= max) base_len = max - 1;
    memcpy(clean, url, base_len);
    int removed = 0, first = 1;
    size_t pos = base_len;
    const char *s = q + 1;
    while (*s && pos < max - 2) {
        const char *eq = strchr(s, '=');
        const char *amp = strchr(s, '&');
        if (!amp) amp = s + strlen(s);
        size_t plen = eq ? (size_t)(eq - s) : (size_t)(amp - s);
        char pname[64] = {0};
        if (plen < sizeof(pname)) memcpy(pname, s, plen);
        if (!eb_priv_is_tracking_param(p, pname)) {
            clean[pos++] = first ? '?' : '&'; first = 0;
            size_t seg = (size_t)(amp - s);
            if (pos + seg < max) { memcpy(clean + pos, s, seg); pos += seg; }
        } else { removed++; p->trackers_stripped++; }
        s = *amp ? amp + 1 : amp;
    }
    clean[pos] = '\0';
    return removed;
}

bool eb_priv_should_allow_cookie(const eb_privacy_t *p, const char *dom,
                                  const char *page, bool tp) {
    if (!p) return true;
    if (p->mode == EB_PRIV_INCOGNITO || p->mode == EB_PRIV_TOR_MODE) {
        if (tp) { return false; }
    }
    switch (p->cookie_policy) {
    case EB_COOKIE_BLOCK_ALL: return false;
    case EB_COOKIE_BLOCK_THIRD_PARTY: if (tp) return false; break;
    default: break;
    }
    if (dom && eb_priv_is_exception(p, dom)) return true;
    return true;
}

int eb_priv_apply_referrer(const eb_privacy_t *p, const char *from,
                            const char *to, char *ref, size_t max) {
    if (!p || !from || !to || !ref) return -1;
    switch (p->referrer_policy) {
    case EB_REF_NONE: ref[0] = '\0'; p->referrers_stripped; return 0;
    case EB_REF_ORIGIN_ONLY: {
        const char *s = strstr(from, "://");
        if (!s) { ref[0]='\0'; return 0; }
        const char *e = strchr(s+3, '/');
        size_t len = e ? (size_t)(e-from) : strlen(from);
        if (len >= max) len = max-1;
        memcpy(ref, from, len); ref[len]='/'; ref[len+1]='\0'; return 0;
    }
    case EB_REF_SAME_ORIGIN: {
        /* Only send referrer if same origin */
        const char *fs = strstr(from, "://"), *ts = strstr(to, "://");
        if (!fs || !ts) { ref[0]='\0'; return 0; }
        const char *fe = strchr(fs+3, '/'), *te = strchr(ts+3, '/');
        size_t fl = fe ? (size_t)(fe-from) : strlen(from);
        size_t tl = te ? (size_t)(te-to) : strlen(to);
        if (fl != tl || strncmp(from, to, fl) != 0) { ref[0]='\0'; return 0; }
        strncpy(ref, from, max-1); return 0;
    }
    default: strncpy(ref, from, max-1); return 0;
    }
}

void eb_priv_inject_headers(const eb_privacy_t *p, const char *url,
                             char *hdrs, size_t max) {
    if (!p || !hdrs) return;
    size_t pos = strlen(hdrs);
    if (p->dnt_enabled && pos + 20 < max) {
        strcpy(hdrs+pos, "DNT: 1\r\n"); pos += 8;
    }
    if (p->gpc_enabled && pos + 30 < max) {
        strcpy(hdrs+pos, "Sec-GPC: 1\r\n"); pos += 12;
    }
}

bool eb_priv_add_exception(eb_privacy_t *p, const char *dom) {
    if (!p || !dom || p->exception_count >= EB_PRIV_MAX_EXCEPTIONS) return false;
    strncpy(p->exceptions[p->exception_count++], dom, 255);
    return true;
}

bool eb_priv_is_exception(const eb_privacy_t *p, const char *dom) {
    if (!p || !dom) return false;
    for (int i = 0; i < p->exception_count; i++)
        if (strcasecmp(p->exceptions[i], dom) == 0) return true;
    return false;
}
