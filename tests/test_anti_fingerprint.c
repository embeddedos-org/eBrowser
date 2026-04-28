// SPDX-License-Identifier: MIT
#include "eBrowser/anti_fingerprint.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static int s_pass = 0, s_fail = 0;
#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %s... ", #name); name(); printf("PASS\n"); s_pass++; } while(0)
#define ASSERT(cond) do { if(!(cond)) { printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); s_fail++; return; } } while(0)

TEST(test_init_defaults) {
    eb_anti_fingerprint_t afp;
    eb_afp_init(&afp);
    ASSERT(afp.level == EB_AFP_STANDARD);
    ASSERT(afp.canvas_noise == true);
    ASSERT(afp.block_webrtc_ip == true);
    ASSERT(afp.block_battery_api == true);
    ASSERT(afp.restrict_fonts == true);
    ASSERT(afp.font_count > 0);
    eb_afp_destroy(&afp);
}

TEST(test_level_off) {
    eb_anti_fingerprint_t afp;
    eb_afp_init(&afp);
    eb_afp_set_level(&afp, EB_AFP_OFF);
    ASSERT(afp.canvas_noise == false);
    ASSERT(afp.webgl_spoof_vendor == false);
    ASSERT(afp.restrict_fonts == false);
    ASSERT(afp.jitter_timing == false);
    ASSERT(afp.block_webrtc_ip == false);
    eb_afp_destroy(&afp);
}

TEST(test_level_strict) {
    eb_anti_fingerprint_t afp;
    eb_afp_init(&afp);
    eb_afp_set_level(&afp, EB_AFP_STRICT);
    ASSERT(afp.canvas_noise_bits == 4);
    ASSERT(afp.normalize_screen == true);
    ASSERT(afp.reported_width == 1920);
    ASSERT(afp.reported_height == 1080);
    ASSERT(afp.spoof_cores == true);
    ASSERT(afp.reported_cores == 4);
    ASSERT(afp.spoof_memory == true);
    ASSERT(afp.spoof_ua == true);
    eb_afp_destroy(&afp);
}

TEST(test_level_tor) {
    eb_anti_fingerprint_t afp;
    eb_afp_init(&afp);
    eb_afp_set_level(&afp, EB_AFP_TOR_LIKE);
    ASSERT(afp.canvas_readback_block == true);
    ASSERT(afp.reported_width == 1000);
    ASSERT(afp.reported_height == 1000);
    ASSERT(afp.reported_cores == 2);
    ASSERT(afp.timing_resolution_ms == 100);
    eb_afp_destroy(&afp);
}

TEST(test_canvas_noise_modifies_pixels) {
    eb_anti_fingerprint_t afp;
    eb_afp_init(&afp);
    eb_afp_set_level(&afp, EB_AFP_STANDARD);
    uint8_t original[64], noised[64];
    memset(original, 128, sizeof(original));
    memcpy(noised, original, sizeof(noised));
    eb_afp_inject_canvas_noise(&afp, noised, 4, 4, 4);
    /* At least some pixels should be changed */
    int changed = 0;
    for (int i = 0; i < 64; i++)
        if (noised[i] != original[i]) changed++;
    ASSERT(changed > 0);
    /* Values should still be close to original (noise is small) */
    for (int i = 0; i < 64; i++) {
        int diff = (int)noised[i] - (int)original[i];
        ASSERT(diff >= -8 && diff <= 8); /* 2-bit noise = max +-1, but check wider */
    }
    eb_afp_destroy(&afp);
}

TEST(test_canvas_noise_off) {
    eb_anti_fingerprint_t afp;
    eb_afp_init(&afp);
    afp.canvas_noise = false;
    uint8_t px[16]; memset(px, 100, sizeof(px));
    uint8_t orig[16]; memcpy(orig, px, sizeof(px));
    eb_afp_inject_canvas_noise(&afp, px, 2, 2, 4);
    ASSERT(memcmp(px, orig, sizeof(px)) == 0); /* Unchanged */
    eb_afp_destroy(&afp);
}

TEST(test_block_canvas_read) {
    eb_anti_fingerprint_t afp;
    eb_afp_init(&afp);
    eb_afp_set_level(&afp, EB_AFP_STANDARD);
    ASSERT(eb_afp_should_block_canvas_read(&afp) == false);
    eb_afp_set_level(&afp, EB_AFP_TOR_LIKE);
    ASSERT(eb_afp_should_block_canvas_read(&afp) == true);
    eb_afp_destroy(&afp);
}

TEST(test_webgl_spoofing) {
    eb_anti_fingerprint_t afp;
    eb_afp_init(&afp);
    const char *v = eb_afp_get_webgl_vendor(&afp);
    ASSERT(v != NULL);
    ASSERT(strcmp(v, "WebKit") == 0);
    /* When off */
    afp.webgl_spoof_vendor = false;
    ASSERT(eb_afp_get_webgl_vendor(&afp) == NULL);
    eb_afp_destroy(&afp);
}

TEST(test_timing_jitter) {
    eb_anti_fingerprint_t afp;
    eb_afp_init(&afp);
    afp.jitter_timing = true;
    afp.timing_jitter_us = 500;
    afp.timing_resolution_ms = 1;
    double t1 = eb_afp_jitter_time(&afp, 1000.0);
    double t2 = eb_afp_jitter_time(&afp, 1000.0);
    /* Results should be close to 1000 but may differ due to jitter */
    ASSERT(fabs(t1 - 1000.0) < 2.0); /* Within 1ms */
    /* With jitter off, should return exact value */
    afp.jitter_timing = false;
    double t3 = eb_afp_jitter_time(&afp, 1000.0);
    ASSERT(t3 == 1000.0);
    eb_afp_destroy(&afp);
}

TEST(test_font_restriction) {
    eb_anti_fingerprint_t afp;
    eb_afp_init(&afp);
    ASSERT(eb_afp_is_font_allowed(&afp, "Arial") == true);
    ASSERT(eb_afp_is_font_allowed(&afp, "serif") == true);
    ASSERT(eb_afp_is_font_allowed(&afp, "Comic Sans MS") == false);
    ASSERT(eb_afp_is_font_allowed(&afp, "Papyrus") == false);
    /* When restriction is off */
    afp.restrict_fonts = false;
    ASSERT(eb_afp_is_font_allowed(&afp, "Any Font") == true);
    eb_afp_destroy(&afp);
}

TEST(test_ua_spoofing) {
    eb_anti_fingerprint_t afp;
    eb_afp_init(&afp);
    eb_afp_set_level(&afp, EB_AFP_STRICT);
    const char *ua = eb_afp_get_ua(&afp);
    ASSERT(ua != NULL);
    ASSERT(strlen(ua) > 10);
    /* When off */
    afp.spoof_ua = false;
    ASSERT(eb_afp_get_ua(&afp) == NULL);
    eb_afp_destroy(&afp);
}

TEST(test_ua_randomize) {
    eb_anti_fingerprint_t afp;
    eb_afp_init(&afp);
    afp.spoof_ua = true;
    eb_afp_randomize_ua(&afp);
    const char *ua = eb_afp_get_ua(&afp);
    ASSERT(ua != NULL);
    ASSERT(strstr(ua, "Mozilla") != NULL);
    eb_afp_destroy(&afp);
}

TEST(test_screen_normalization) {
    eb_anti_fingerprint_t afp;
    eb_afp_init(&afp);
    eb_afp_set_level(&afp, EB_AFP_STRICT);
    int w, h, d;
    eb_afp_get_screen(&afp, &w, &h, &d);
    ASSERT(w == 1920);
    ASSERT(h == 1080);
    ASSERT(d == 24);
    eb_afp_destroy(&afp);
}

TEST(test_entropy_score) {
    eb_anti_fingerprint_t afp;
    eb_afp_init(&afp);
    eb_afp_set_level(&afp, EB_AFP_STANDARD);
    eb_afp_score_t score;
    eb_afp_calculate_score(&afp, &score);
    ASSERT(score.total_entropy < 0.5f); /* Should be low with protections on */
    ASSERT(strcmp(score.risk, "LOW") == 0);

    eb_afp_set_level(&afp, EB_AFP_OFF);
    eb_afp_calculate_score(&afp, &score);
    ASSERT(score.total_entropy > 0.8f); /* High without protections */
    ASSERT(strcmp(score.risk, "HIGH") == 0);
    eb_afp_destroy(&afp);
}

TEST(test_null_safety) {
    eb_afp_init(NULL);
    eb_afp_set_level(NULL, EB_AFP_STRICT);
    eb_afp_inject_canvas_noise(NULL, NULL, 0, 0, 0);
    ASSERT(eb_afp_should_block_canvas_read(NULL) == false);
    ASSERT(eb_afp_get_webgl_vendor(NULL) == NULL);
    ASSERT(eb_afp_get_ua(NULL) == NULL);
    ASSERT(eb_afp_is_font_allowed(NULL, "Arial") == true); /* NULL = no restriction */
    eb_afp_destroy(NULL);
}

int main(void) {
    printf("=== Anti-Fingerprint Tests ===\n");
    RUN(test_init_defaults); RUN(test_level_off);
    RUN(test_level_strict); RUN(test_level_tor);
    RUN(test_canvas_noise_modifies_pixels); RUN(test_canvas_noise_off);
    RUN(test_block_canvas_read); RUN(test_webgl_spoofing);
    RUN(test_timing_jitter); RUN(test_font_restriction);
    RUN(test_ua_spoofing); RUN(test_ua_randomize);
    RUN(test_screen_normalization); RUN(test_entropy_score);
    RUN(test_null_safety);
    printf("\nResults: %d passed, %d failed\n", s_pass, s_fail);
    return s_fail > 0 ? 1 : 0;
}
