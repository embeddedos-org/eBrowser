// SPDX-License-Identifier: MIT
#include "eBrowser/privacy.h"
#include "eBrowser/tracker_blocker.h"
#include <stdio.h>
#include <string.h>

static int s_pass = 0, s_fail = 0;
#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %s... ", #name); name(); printf("PASS\n"); s_pass++; } while(0)
#define ASSERT(cond) do { if(!(cond)) { printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); s_fail++; return; } } while(0)

/* --- Privacy --- */
TEST(test_priv_init) {
    eb_privacy_t p;
    eb_priv_init(&p);
    ASSERT(p.dnt_enabled == true);
    ASSERT(p.gpc_enabled == true);
    ASSERT(p.strip_tracking_params == true);
    ASSERT(p.https_only == true);
    ASSERT(p.tracking_param_count > 0);
    eb_priv_destroy(&p);
}

TEST(test_tracking_params_default) {
    eb_privacy_t p;
    eb_priv_init(&p);
    ASSERT(eb_priv_is_tracking_param(&p, "utm_source") == true);
    ASSERT(eb_priv_is_tracking_param(&p, "fbclid") == true);
    ASSERT(eb_priv_is_tracking_param(&p, "gclid") == true);
    ASSERT(eb_priv_is_tracking_param(&p, "msclkid") == true);
    ASSERT(eb_priv_is_tracking_param(&p, "page") == false);
    ASSERT(eb_priv_is_tracking_param(&p, "q") == false);
    eb_priv_destroy(&p);
}

TEST(test_clean_url_strips_tracking) {
    eb_privacy_t p;
    eb_priv_init(&p);
    char clean[1024];
    int removed = eb_priv_clean_url(&p,
        "https://example.com/page?q=search&utm_source=google&utm_medium=cpc&ref=home",
        clean, sizeof(clean));
    ASSERT(removed == 3); /* utm_source, utm_medium, ref */
    ASSERT(strstr(clean, "utm_source") == NULL);
    ASSERT(strstr(clean, "utm_medium") == NULL);
    ASSERT(strstr(clean, "q=search") != NULL);
    ASSERT(strstr(clean, "ref=home") == NULL); /* ref is in default list */
    eb_priv_destroy(&p);
}

TEST(test_clean_url_no_params) {
    eb_privacy_t p;
    eb_priv_init(&p);
    char clean[1024];
    int removed = eb_priv_clean_url(&p, "https://example.com/page", clean, sizeof(clean));
    ASSERT(removed == 0);
    ASSERT(strcmp(clean, "https://example.com/page") == 0);
    eb_priv_destroy(&p);
}

TEST(test_clean_url_all_tracking) {
    eb_privacy_t p;
    eb_priv_init(&p);
    char clean[1024];
    int removed = eb_priv_clean_url(&p,
        "https://example.com/?fbclid=abc&gclid=xyz",
        clean, sizeof(clean));
    ASSERT(removed == 2);
    ASSERT(strcmp(clean, "https://example.com/") == 0);
    eb_priv_destroy(&p);
}

TEST(test_cookie_policy_block_third_party) {
    eb_privacy_t p;
    eb_priv_init(&p);
    p.cookie_policy = EB_COOKIE_BLOCK_THIRD_PARTY;
    ASSERT(eb_priv_should_allow_cookie(&p, "example.com", "example.com", false) == true);
    ASSERT(eb_priv_should_allow_cookie(&p, "tracker.com", "example.com", true) == false);
    eb_priv_destroy(&p);
}

TEST(test_cookie_policy_block_all) {
    eb_privacy_t p;
    eb_priv_init(&p);
    p.cookie_policy = EB_COOKIE_BLOCK_ALL;
    ASSERT(eb_priv_should_allow_cookie(&p, "example.com", "example.com", false) == false);
    eb_priv_destroy(&p);
}

TEST(test_incognito_mode) {
    eb_privacy_t p;
    eb_priv_init(&p);
    eb_priv_set_mode(&p, EB_PRIV_INCOGNITO);
    ASSERT(p.mode == EB_PRIV_INCOGNITO);
    ASSERT(p.clear_on_exit == true);
    ASSERT(eb_priv_should_allow_cookie(&p, "tracker.com", "page.com", true) == false);
    eb_priv_destroy(&p);
}

TEST(test_tor_mode) {
    eb_privacy_t p;
    eb_priv_init(&p);
    eb_priv_set_mode(&p, EB_PRIV_TOR_MODE);
    ASSERT(p.mode == EB_PRIV_TOR_MODE);
    ASSERT(p.fpi_enabled == true);
    ASSERT(p.storage_partition == true);
    ASSERT(p.cookie_policy == EB_COOKIE_BLOCK_ALL);
    ASSERT(p.referrer_policy == EB_REF_NONE);
    eb_priv_destroy(&p);
}

TEST(test_referrer_none) {
    eb_privacy_t p;
    eb_priv_init(&p);
    p.referrer_policy = EB_REF_NONE;
    char ref[256];
    eb_priv_apply_referrer(&p, "https://source.com/page", "https://dest.com", ref, sizeof(ref));
    ASSERT(ref[0] == '\0');
    eb_priv_destroy(&p);
}

TEST(test_referrer_origin_only) {
    eb_privacy_t p;
    eb_priv_init(&p);
    p.referrer_policy = EB_REF_ORIGIN_ONLY;
    char ref[256];
    eb_priv_apply_referrer(&p, "https://source.com/secret/page?key=123", "https://dest.com", ref, sizeof(ref));
    ASSERT(strcmp(ref, "https://source.com/") == 0);
    eb_priv_destroy(&p);
}

TEST(test_exceptions) {
    eb_privacy_t p;
    eb_priv_init(&p);
    ASSERT(eb_priv_is_exception(&p, "trusted.com") == false);
    ASSERT(eb_priv_add_exception(&p, "trusted.com") == true);
    ASSERT(eb_priv_is_exception(&p, "trusted.com") == true);
    ASSERT(eb_priv_is_exception(&p, "other.com") == false);
    eb_priv_destroy(&p);
}

TEST(test_header_injection) {
    eb_privacy_t p;
    eb_priv_init(&p);
    char headers[1024] = "";
    eb_priv_inject_headers(&p, "https://example.com", headers, sizeof(headers));
    ASSERT(strstr(headers, "DNT: 1") != NULL);
    ASSERT(strstr(headers, "Sec-GPC: 1") != NULL);
    eb_priv_destroy(&p);
}

/* --- Tracker Blocker --- */
TEST(test_tb_init) {
    static eb_tracker_blocker_t tb;
    eb_tb_init(&tb);
    ASSERT(tb.enabled == true);
    ASSERT(tb.block_third_party_cookies == true);
    ASSERT(tb.strip_referrer == true);
    ASSERT(tb.filter_count == 0);
    eb_tb_destroy(&tb);
}

TEST(test_tb_parse_domain_filter) {
    static eb_tracker_blocker_t tb;
    eb_tb_init(&tb);
    int r = eb_tb_parse_filter_line(&tb, "||doubleclick.net^");
    ASSERT(r == 1);
    ASSERT(tb.filter_count == 1);
    ASSERT(tb.filters[0].action == EB_TB_BLOCK);
    ASSERT(tb.filters[0].enabled == true);
    eb_tb_destroy(&tb);
}

TEST(test_tb_parse_exception) {
    static eb_tracker_blocker_t tb;
    eb_tb_init(&tb);
    eb_tb_parse_filter_line(&tb, "@@||allowed.com^");
    ASSERT(tb.filter_count == 1);
    ASSERT(tb.filters[0].action == EB_TB_ALLOW);
    eb_tb_destroy(&tb);
}

TEST(test_tb_parse_comment) {
    static eb_tracker_blocker_t tb;
    eb_tb_init(&tb);
    ASSERT(eb_tb_parse_filter_line(&tb, "! This is a comment") == 0);
    ASSERT(eb_tb_parse_filter_line(&tb, "[Adblock Plus 2.0]") == 0);
    ASSERT(eb_tb_parse_filter_line(&tb, "") == 0);
    ASSERT(tb.filter_count == 0);
    eb_tb_destroy(&tb);
}

TEST(test_tb_parse_cosmetic) {
    static eb_tracker_blocker_t tb;
    eb_tb_init(&tb);
    eb_tb_parse_filter_line(&tb, "example.com##.ad-banner");
    ASSERT(tb.cosmetic_count == 1);
    ASSERT(strcmp(tb.cosmetics[0].selector, ".ad-banner") == 0);
    ASSERT(strcmp(tb.cosmetics[0].domain, "example.com") == 0);
    eb_tb_destroy(&tb);
}

TEST(test_tb_block_tracker) {
    static eb_tracker_blocker_t tb;
    eb_tb_init(&tb);
    eb_tb_parse_filter_line(&tb, "||tracker.example.com^");
    ASSERT(eb_tb_should_block(&tb, "https://tracker.example.com/pixel.gif",
        "https://mysite.com", EB_TB_RES_IMAGE) == true);
    ASSERT(eb_tb_should_block(&tb, "https://safe.example.com/image.png",
        "https://mysite.com", EB_TB_RES_IMAGE) == false);
    ASSERT(tb.total_blocked == 1);
    ASSERT(tb.total_allowed == 1);
    eb_tb_destroy(&tb);
}

TEST(test_tb_exception_overrides_block) {
    static eb_tracker_blocker_t tb;
    eb_tb_init(&tb);
    eb_tb_parse_filter_line(&tb, "||ads.example.com^");
    eb_tb_parse_filter_line(&tb, "@@||ads.example.com^");
    /* Exception for entire domain overrides the block */
    ASSERT(eb_tb_should_block(&tb, "https://ads.example.com/allowed/banner",
        "https://site.com", EB_TB_RES_IMAGE) == false);
    eb_tb_destroy(&tb);
}

TEST(test_tb_add_remove_filter) {
    static eb_tracker_blocker_t tb;
    eb_tb_init(&tb);
    ASSERT(eb_tb_add_custom_filter(&tb, "||custom-tracker.com^") == true);
    ASSERT(tb.filter_count == 1);
    ASSERT(eb_tb_remove_filter(&tb, "||custom-tracker.com^") == true);
    ASSERT(tb.filter_count == 0);
    ASSERT(eb_tb_remove_filter(&tb, "nonexistent") == false);
    eb_tb_destroy(&tb);
}

TEST(test_tb_disabled) {
    static eb_tracker_blocker_t tb;
    eb_tb_init(&tb);
    tb.enabled = false;
    eb_tb_parse_filter_line(&tb, "||tracker.com^");
    ASSERT(eb_tb_should_block(&tb, "https://tracker.com/ad.js",
        "https://site.com", EB_TB_RES_SCRIPT) == false);
    eb_tb_destroy(&tb);
}

TEST(test_tb_stats) {
    static eb_tracker_blocker_t tb;
    eb_tb_init(&tb);
    eb_tb_parse_filter_line(&tb, "||ad.com^");
    eb_tb_should_block(&tb, "https://ad.com/x", "https://s.com", EB_TB_RES_SCRIPT);
    eb_tb_should_block(&tb, "https://ok.com/y", "https://s.com", EB_TB_RES_IMAGE);
    eb_tb_stats_t stats;
    eb_tb_get_stats(&tb, &stats);
    ASSERT(stats.blocked == 1);
    ASSERT(stats.allowed == 1);
    ASSERT(stats.filters == 1);
    eb_tb_destroy(&tb);
}

int main(void) {
    printf("=== Privacy & Tracker Blocker Tests ===\n");
    RUN(test_priv_init); RUN(test_tracking_params_default);
    RUN(test_clean_url_strips_tracking); RUN(test_clean_url_no_params);
    RUN(test_clean_url_all_tracking);
    RUN(test_cookie_policy_block_third_party); RUN(test_cookie_policy_block_all);
    RUN(test_incognito_mode); RUN(test_tor_mode);
    RUN(test_referrer_none); RUN(test_referrer_origin_only);
    RUN(test_exceptions); RUN(test_header_injection);
    RUN(test_tb_init); RUN(test_tb_parse_domain_filter);
    RUN(test_tb_parse_exception); RUN(test_tb_parse_comment);
    RUN(test_tb_parse_cosmetic); RUN(test_tb_block_tracker);
    RUN(test_tb_exception_overrides_block); RUN(test_tb_add_remove_filter);
    RUN(test_tb_disabled); RUN(test_tb_stats);
    printf("\nResults: %d passed, %d failed\n", s_pass, s_fail);
    return s_fail > 0 ? 1 : 0;
}
