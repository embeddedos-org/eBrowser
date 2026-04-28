// SPDX-License-Identifier: MIT
#ifndef eBrowser_PRIVACY_H
#define eBrowser_PRIVACY_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define EB_PRIV_MAX_TRACKING_PARAMS 64
#define EB_PRIV_MAX_PARAM_LEN       64
#define EB_PRIV_MAX_EXCEPTIONS      128

typedef enum { EB_PRIV_NORMAL, EB_PRIV_INCOGNITO, EB_PRIV_TOR_MODE } eb_priv_mode_t;
typedef enum { EB_COOKIE_KEEP_ALL, EB_COOKIE_BLOCK_THIRD_PARTY, EB_COOKIE_BLOCK_ALL, EB_COOKIE_DELETE_ON_CLOSE } eb_cookie_policy_t;
typedef enum { EB_REF_FULL, EB_REF_ORIGIN_ONLY, EB_REF_SAME_ORIGIN, EB_REF_NONE } eb_referrer_policy_t;

typedef struct {
    eb_priv_mode_t mode; eb_cookie_policy_t cookie_policy; eb_referrer_policy_t referrer_policy;
    bool dnt_enabled, gpc_enabled, fpi_enabled, storage_partition;
    bool strip_tracking_params, block_bounce_tracking, https_only;
    bool block_third_party_cookies, clear_on_exit;
    char tracking_params[EB_PRIV_MAX_TRACKING_PARAMS][EB_PRIV_MAX_PARAM_LEN]; int tracking_param_count;
    char exceptions[EB_PRIV_MAX_EXCEPTIONS][256]; int exception_count;
    uint64_t cookies_blocked, trackers_stripped, referrers_stripped;
} eb_privacy_t;

void eb_priv_init(eb_privacy_t *priv);
void eb_priv_destroy(eb_privacy_t *priv);
void eb_priv_set_mode(eb_privacy_t *priv, eb_priv_mode_t mode);
int  eb_priv_clean_url(eb_privacy_t *priv, const char *url, char *clean, size_t max);
bool eb_priv_is_tracking_param(const eb_privacy_t *priv, const char *param);
void eb_priv_add_tracking_param(eb_privacy_t *priv, const char *param);
bool eb_priv_should_allow_cookie(const eb_privacy_t *priv, const char *domain, const char *page, bool third_party);
int  eb_priv_apply_referrer(const eb_privacy_t *priv, const char *from, const char *to, char *ref, size_t max);
void eb_priv_inject_headers(const eb_privacy_t *priv, const char *url, char *headers, size_t max);
bool eb_priv_add_exception(eb_privacy_t *priv, const char *domain);
bool eb_priv_is_exception(const eb_privacy_t *priv, const char *domain);
#endif
