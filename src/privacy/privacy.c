// SPDX-License-Identifier: MIT
#include "eBrowser/privacy.h"
#include "eBrowser/compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

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
    /* Init default EB_AFP_STANDARD == the EB_PRIV_NORMAL mapping, so a
     * freshly-initialized engine is already at the right AFP level. */
    eb_afp_init(&p->afp);
}

void eb_priv_destroy(eb_privacy_t *p) { if (p) { eb_afp_destroy(&p->afp); memset(p, 0, sizeof(*p)); } }

/* Decided mode → AFP-level mapping (docs/privacy-invariants.md). Unknown
 * modes fall back to STANDARD — the init default, never the weakest. */
static eb_afp_level_t eb_priv_afp_for_mode(eb_priv_mode_t m) {
    switch (m) {
    case EB_PRIV_INCOGNITO: return EB_AFP_STRICT;
    case EB_PRIV_TOR_MODE:  return EB_AFP_TOR_LIKE;
    case EB_PRIV_NORMAL:
    default:                return EB_AFP_STANDARD;
    }
}

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
    eb_afp_set_level(&p->afp, eb_priv_afp_for_mode(m));
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

static bool referrer_copy(char *dst, const char *src, size_t max) {
    size_t len = strlen(src);
    if (len >= max) return false;
    memcpy(dst, src, len + 1);
    return true;
}

int eb_priv_apply_referrer(const eb_privacy_t *p, const char *from,
                            const char *to, char *ref, size_t max) {
    if (!p || !from || !to || !ref || max == 0) return -1;
    /* Referrer truncation can disclose a malformed partial URL. Start from no
     * referrer and populate it only when the complete policy result fits. */
    ref[0] = '\0';
    switch (p->referrer_policy) {
    /* p->referrers_stripped is not incremented here: p is const, so the
     * statement that used to sit on this line had no effect and the counter
     * has never moved. Fixing that means changing this function's signature,
     * which is a public-API decision rather than a bounds fix. */
    case EB_REF_NONE: return 0;
    case EB_REF_FULL:
        referrer_copy(ref, from, max);
        return 0;
    case EB_REF_ORIGIN_ONLY: {
        const char *s = strstr(from, "://");
        if (!s) return 0;
        const char *e = strchr(s+3, '/');
        size_t len = e ? (size_t)(e-from) : strlen(from);
        /* Reserve the trailing slash and NUL. If the complete origin does not
         * fit, keep the fail-closed empty referrer instead of truncating it. */
        if (len > max - 1 || len + 1 >= max) return 0;
        memcpy(ref, from, len); ref[len]='/'; ref[len+1]='\0';
        return 0;
    }
    case EB_REF_SAME_ORIGIN: {
        /* Only send referrer if same origin */
        const char *fs = strstr(from, "://"), *ts = strstr(to, "://");
        if (!fs || !ts) return 0;
        const char *fe = strchr(fs+3, '/'), *te = strchr(ts+3, '/');
        size_t fl = fe ? (size_t)(fe-from) : strlen(from);
        size_t tl = te ? (size_t)(te-to) : strlen(to);
        if (fl != tl || strncmp(from, to, fl) != 0) return 0;
        referrer_copy(ref, from, max);
        return 0;
    }
    default:
        return 0;
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
