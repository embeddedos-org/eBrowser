// SPDX-License-Identifier: MIT
// Privacy invariants for eBrowser: properties that must hold no matter how
// the privacy code evolves. Tests the real src/privacy/ implementation.
//
// Invariants (not examples):
//  - every known tracking query param is stripped from cleaned URLs
//  - no cleaned URL ever retains a tracking param
//  - third-party cookies are denied in incognito/Tor regardless of policy
//  - referrer policies never leak more than the policy allows
//  - the tracker blocker denies what its filters match and counts it
//  - each privacy mode sets its AFP ladder level; raising mode never lowers it
#include "eBrowser/privacy.h"
#include "eBrowser/tracker_blocker.h"
#include <stdio.h>
#include <string.h>

static int s_pass = 0, s_fail = 0;
#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %s... ", #name); name(); printf("PASS\n"); s_pass++; } while(0)
#define ASSERT(cond) do { if(!(cond)) { printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); s_fail++; return; } } while(0)

/* eb_tracker_blocker_t is ~16MB: it must live in static storage, never on
 * the stack. */
static eb_tracker_blocker_t s_tb;

TEST(test_init_defaults) {
    eb_privacy_t p;
    eb_priv_init(&p);
    ASSERT(p.mode == EB_PRIV_NORMAL);
    ASSERT(p.cookie_policy == EB_COOKIE_BLOCK_THIRD_PARTY);
    ASSERT(p.strip_tracking_params == true);
    ASSERT(p.tracking_param_count > 0);
    eb_priv_destroy(&p);
}

/* Invariant: EVERY param in the default tracking list is stripped, and no
 * cleaned URL retains one. Derived from the live list, so adding a new
 * default tracker automatically extends this test. */
TEST(test_url_cleaning_strips_every_default_tracker) {
    eb_privacy_t p;
    eb_priv_init(&p);
    ASSERT(p.tracking_param_count > 0);
    char url[4096], clean[4096];
    strcpy(url, "https://ex.com/page?id=5");
    for (int i = 0; i < p.tracking_param_count; i++) {
        char tmp[96];
        snprintf(tmp, sizeof tmp, "&%s=x", p.tracking_params[i]);
        strcat(url, tmp);
    }
    int removed = eb_priv_clean_url(&p, url, clean, sizeof clean);
    ASSERT(removed == p.tracking_param_count);
    ASSERT(strcmp(clean, "https://ex.com/page?id=5") == 0);
    for (int i = 0; i < p.tracking_param_count; i++) {
        char needle[96];
        snprintf(needle, sizeof needle, "%s=", p.tracking_params[i]);
        ASSERT(strstr(clean, needle) == NULL);
        ASSERT(eb_priv_is_tracking_param(&p, p.tracking_params[i]) == true);
    }
    ASSERT(eb_priv_is_tracking_param(&p, "id") == false);
    eb_priv_destroy(&p);
}

TEST(test_url_cleaning_disabled_passes_through) {
    eb_privacy_t p;
    eb_priv_init(&p);
    p.strip_tracking_params = false;
    char clean[1024];
    const char *url = "https://ex.com/?utm_source=x&id=1";
    int removed = eb_priv_clean_url(&p, url, clean, sizeof clean);
    ASSERT(removed == 0);
    ASSERT(strcmp(clean, url) == 0);
    eb_priv_destroy(&p);
}

TEST(test_cookie_third_party_blocked_by_default) {
    eb_privacy_t p;
    eb_priv_init(&p);
    ASSERT(eb_priv_should_allow_cookie(&p, "t.com", "p.com", true) == false);
    ASSERT(eb_priv_should_allow_cookie(&p, "p.com", "p.com", false) == true);
    eb_priv_destroy(&p);
}

TEST(test_cookie_block_all_denies_everything) {
    eb_privacy_t p;
    eb_priv_init(&p);
    p.cookie_policy = EB_COOKIE_BLOCK_ALL;
    ASSERT(eb_priv_should_allow_cookie(&p, "p.com", "p.com", false) == false);
    ASSERT(eb_priv_should_allow_cookie(&p, "t.com", "p.com", true) == false);
    eb_priv_destroy(&p);
}

TEST(test_cookie_incognito_blocks_third_party_regardless_of_policy) {
    eb_privacy_t p;
    eb_priv_init(&p);
    /* Invariant: private modes deny third-party cookies even when the
     * policy would keep them. */
    p.cookie_policy = EB_COOKIE_KEEP_ALL;
    eb_priv_set_mode(&p, EB_PRIV_INCOGNITO);
    ASSERT(eb_priv_should_allow_cookie(&p, "t.com", "p.com", true) == false);
    eb_priv_set_mode(&p, EB_PRIV_TOR_MODE);
    ASSERT(eb_priv_should_allow_cookie(&p, "t.com", "p.com", true) == false);
    eb_priv_destroy(&p);
}

TEST(test_referrer_none_is_empty) {
    eb_privacy_t p;
    eb_priv_init(&p);
    p.referrer_policy = EB_REF_NONE;
    char ref[256];
    memset(ref, 'X', sizeof ref);
    ASSERT(eb_priv_apply_referrer(&p, "https://a.com/secret", "https://b.com/",
                                  ref, sizeof ref) == 0);
    ASSERT(ref[0] == '\0');
    eb_priv_destroy(&p);
}

TEST(test_referrer_origin_only_strips_path_and_query) {
    eb_privacy_t p;
    eb_priv_init(&p);
    p.referrer_policy = EB_REF_ORIGIN_ONLY;
    char ref[256];
    ASSERT(eb_priv_apply_referrer(&p, "https://a.com/private/path?token=abc",
                                  "https://b.com/", ref, sizeof ref) == 0);
    ASSERT(strcmp(ref, "https://a.com/") == 0);
    /* Invariant: the path and query never leak through origin-only. */
    ASSERT(strstr(ref, "private") == NULL);
    ASSERT(strstr(ref, "token") == NULL);
    eb_priv_destroy(&p);
}

TEST(test_referrer_same_origin) {
    eb_privacy_t p;
    eb_priv_init(&p);
    p.referrer_policy = EB_REF_SAME_ORIGIN;
    char ref[256];
    /* Cross-origin: nothing sent. */
    ASSERT(eb_priv_apply_referrer(&p, "https://a.com/x", "https://b.com/y",
                                  ref, sizeof ref) == 0);
    ASSERT(ref[0] == '\0');
    /* Same-origin: full referrer sent. */
    ASSERT(eb_priv_apply_referrer(&p, "https://a.com/x", "https://a.com/y",
                                  ref, sizeof ref) == 0);
    ASSERT(strcmp(ref, "https://a.com/x") == 0);
    eb_priv_destroy(&p);
}

TEST(test_dnt_gpc_headers) {
    eb_privacy_t p;
    eb_priv_init(&p);
    char hdrs[256] = {0};
    eb_priv_inject_headers(&p, "https://a.com", hdrs, sizeof hdrs);
    /* Defaults enable both signals. */
    ASSERT(strstr(hdrs, "DNT: 1") != NULL);
    ASSERT(strstr(hdrs, "Sec-GPC: 1") != NULL);
    eb_priv_destroy(&p);
}

TEST(test_tracker_blocker_blocks_and_counts) {
    eb_tb_init(&s_tb);
    ASSERT(s_tb.enabled == true);
    ASSERT(eb_tb_add_custom_filter(&s_tb, "||tracker.example.com^") == true);
    ASSERT(eb_tb_should_block(&s_tb, "https://tracker.example.com/pixel.js",
                              "example.org", EB_TB_RES_SCRIPT) == true);
    /* Invariant: non-matching URLs are never blocked. */
    ASSERT(eb_tb_should_block(&s_tb, "https://example.org/app.js",
                              "example.org", EB_TB_RES_SCRIPT) == false);
    eb_tb_stats_t st;
    eb_tb_get_stats(&s_tb, &st);
    ASSERT(st.blocked == 1);
    ASSERT(st.allowed == 1);
    ASSERT(st.filters == 1);
    eb_tb_destroy(&s_tb);
}

/* Privacy mode -> anti-fingerprinting ladder wiring
 * (docs/privacy-invariants.md, decided 2026-10-11):
 *   EB_PRIV_NORMAL -> EB_AFP_STANDARD
 *   EB_PRIV_INCOGNITO -> EB_AFP_STRICT
 *   EB_PRIV_TOR_MODE -> EB_AFP_TOR_LIKE */
TEST(test_init_afp_level_matches_normal_mode) {
    eb_privacy_t p;
    eb_priv_init(&p);
    /* The eb_afp_init default is STANDARD, matching the Normal mapping. */
    ASSERT(p.afp.level == EB_AFP_STANDARD);
    eb_priv_destroy(&p);
}

TEST(test_mode_drives_afp_level) {
    eb_privacy_t p;
    eb_priv_init(&p);
    eb_priv_set_mode(&p, EB_PRIV_NORMAL);
    ASSERT(p.afp.level == EB_AFP_STANDARD);
    eb_priv_set_mode(&p, EB_PRIV_INCOGNITO);
    ASSERT(p.afp.level == EB_AFP_STRICT);
    eb_priv_set_mode(&p, EB_PRIV_TOR_MODE);
    ASSERT(p.afp.level == EB_AFP_TOR_LIKE);
    /* Re-entering a mode re-applies its level (no one-way latch). */
    eb_priv_set_mode(&p, EB_PRIV_NORMAL);
    ASSERT(p.afp.level == EB_AFP_STANDARD);
    eb_priv_destroy(&p);
}

/* Invariant: raising the privacy mode never lowers AFP protection. The
 * AFP enum is ordered OFF < STANDARD < STRICT < TOR_LIKE, so the mode
 * ladder Normal -> Incognito -> Tor must be non-decreasing in level. */
TEST(test_mode_raises_never_lower_afp) {
    static const eb_priv_mode_t s_ladder[] = {
        EB_PRIV_NORMAL, EB_PRIV_INCOGNITO, EB_PRIV_TOR_MODE
    };
    eb_privacy_t p;
    eb_priv_init(&p);
    eb_afp_level_t prev = EB_AFP_OFF;
    for (size_t i = 0; i < sizeof(s_ladder)/sizeof(s_ladder[0]); i++) {
        eb_priv_set_mode(&p, s_ladder[i]);
        ASSERT(p.afp.level >= prev);
           prev = p.afp.level;
    }
    eb_priv_destroy(&p);
}

TEST(test_null_safety) {
    eb_priv_init(NULL);
    eb_priv_destroy(NULL);
    char clean[16];
    ASSERT(eb_priv_clean_url(NULL, "https://a.com", clean, sizeof clean) == 0);
    ASSERT(eb_priv_is_tracking_param(NULL, "utm_source") == false);
    ASSERT(eb_priv_should_allow_cookie(NULL, "a", "b", true) == true);
    eb_tb_init(NULL);
    eb_tb_destroy(NULL);
}

int main(void) {
    printf("privacy invariants:\n");
    RUN(test_init_defaults);
    RUN(test_url_cleaning_strips_every_default_tracker);
    RUN(test_url_cleaning_disabled_passes_through);
    RUN(test_cookie_third_party_blocked_by_default);
    RUN(test_cookie_block_all_denies_everything);
    RUN(test_cookie_incognito_blocks_third_party_regardless_of_policy);
    RUN(test_referrer_none_is_empty);
    RUN(test_referrer_origin_only_strips_path_and_query);
    RUN(test_referrer_same_origin);
    RUN(test_dnt_gpc_headers);
    RUN(test_tracker_blocker_blocks_and_counts);
    RUN(test_init_afp_level_matches_normal_mode);
    RUN(test_mode_drives_afp_level);
    RUN(test_mode_raises_never_lower_afp);
    RUN(test_null_safety);
    printf("\nResults: %d passed, %d failed\n", s_pass, s_fail);
    return s_fail > 0 ? 1 : 0;
}
