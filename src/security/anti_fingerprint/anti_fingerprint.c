// SPDX-License-Identifier: MIT
#include "eBrowser/anti_fingerprint.h"
#include "eBrowser/compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static const char *s_default_fonts[] = {
    "serif","sans-serif","monospace","Arial","Courier New",
    "Georgia","Times New Roman","Verdana","Helvetica","DejaVu Sans",NULL
};
static const char *s_ua_pool[] = {
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/120.0.0.0 Safari/537.36",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 14_0) AppleWebKit/537.36 Chrome/120.0.0.0 Safari/537.36",
    "Mozilla/5.0 (X11; Linux x86_64; rv:121.0) Gecko/20100101 Firefox/121.0",
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:121.0) Gecko/20100101 Firefox/121.0",
};
static unsigned int afp_rng = 0;
static unsigned int afp_rand(void) { afp_rng = afp_rng * 1103515245 + 12345; return (afp_rng >> 16) & 0x7FFF; }

void eb_afp_init(eb_anti_fingerprint_t *a) {
    if (!a) return; memset(a, 0, sizeof(*a));
    afp_rng = (unsigned)time(NULL);
    eb_afp_set_level(a, EB_AFP_STANDARD);
}

void eb_afp_set_level(eb_anti_fingerprint_t *a, eb_afp_level_t lv) {
    if (!a) return; a->level = lv;
    switch (lv) {
    case EB_AFP_OFF:
        a->canvas_noise = a->webgl_spoof_vendor = a->restrict_fonts = false;
        a->jitter_timing = a->normalize_screen = a->spoof_ua = false;
        a->block_webrtc_ip = a->audio_noise = false; break;
    case EB_AFP_STANDARD:
        a->canvas_noise = true; a->canvas_noise_bits = 2;
        a->webgl_spoof_vendor = true;
        strcpy(a->webgl_vendor, "WebKit"); strcpy(a->webgl_renderer, "WebKit WebGL");
        a->restrict_fonts = true; a->jitter_timing = true;
        a->timing_jitter_us = 100; a->timing_resolution_ms = 1;
        a->block_webrtc_ip = a->block_battery_api = true;
        a->block_bluetooth_api = a->block_usb_api = true;
        a->audio_noise = true; a->audio_noise_level = 0.0001f; break;
    case EB_AFP_STRICT:
        a->canvas_noise = true; a->canvas_noise_bits = 4;
        a->webgl_spoof_vendor = a->webgl_spoof_renderer = true;
        a->webgl_block_extensions = true;
        strcpy(a->webgl_vendor, "WebKit"); strcpy(a->webgl_renderer, "WebKit WebGL");
        a->restrict_fonts = a->jitter_timing = true;
        a->timing_jitter_us = 500; a->timing_resolution_ms = 5;
        a->normalize_screen = true; a->reported_width = 1920;
        a->reported_height = 1080; a->reported_depth = 24;
        a->spoof_ua = a->spoof_cores = a->spoof_memory = true;
        a->reported_cores = 4; a->reported_memory_gb = 8;
        a->block_webrtc_ip = a->block_battery_api = true;
        a->block_bluetooth_api = a->block_usb_api = true;
        a->audio_noise = true; a->audio_noise_level = 0.001f; break;
    case EB_AFP_TOR_LIKE:
        a->canvas_readback_block = true;
        a->webgl_spoof_vendor = a->webgl_spoof_renderer = true;
        a->webgl_block_extensions = true;
        strcpy(a->webgl_vendor, "Mozilla"); strcpy(a->webgl_renderer, "Mozilla");
        a->restrict_fonts = a->jitter_timing = true;
        a->timing_jitter_us = 1000; a->timing_resolution_ms = 100;
        a->normalize_screen = true; a->reported_width = 1000;
        a->reported_height = 1000; a->reported_depth = 24;
        a->spoof_ua = true; a->randomize_ua_per_session = false;
        strcpy(a->spoofed_ua, s_ua_pool[2]);
        a->spoof_cores = a->spoof_memory = true;
        a->reported_cores = 2; a->reported_memory_gb = 8;
        a->block_webrtc_ip = a->block_battery_api = true;
        a->block_bluetooth_api = a->block_usb_api = true;
        a->audio_noise = true; a->audio_noise_level = 0.01f; break;
    }
    if (a->restrict_fonts) {
        a->font_count = 0;
        for (int i = 0; s_default_fonts[i] && a->font_count < EB_AFP_MAX_FONTS; i++)
            strncpy(a->allowed_fonts[a->font_count++], s_default_fonts[i], 127);
    }
}

void eb_afp_inject_canvas_noise(eb_anti_fingerprint_t *a, uint8_t *px, int w, int h, int ch) {
    if (!a || !a->canvas_noise || !px) return;
    int total = w * h * ch, mask = (1 << a->canvas_noise_bits) - 1;
    for (int i = 0; i < total; i++) {
        int noise = (int)(afp_rand() & mask) - (mask >> 1);
        int v = (int)px[i] + noise;
        px[i] = (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
    }
}

bool eb_afp_should_block_canvas_read(const eb_anti_fingerprint_t *a) {
    return a && a->canvas_readback_block;
}

const char *eb_afp_get_webgl_vendor(const eb_anti_fingerprint_t *a) {
    return (a && a->webgl_spoof_vendor) ? a->webgl_vendor : NULL;
}
const char *eb_afp_get_webgl_renderer(const eb_anti_fingerprint_t *a) {
    return (a && a->webgl_spoof_renderer) ? a->webgl_renderer : NULL;
}

double eb_afp_jitter_time(const eb_anti_fingerprint_t *a, double real_ms) {
    if (!a || !a->jitter_timing) return real_ms;
    double jitter = ((double)(afp_rand() % (a->timing_jitter_us * 2 + 1)) - a->timing_jitter_us) / 1000.0;
    double result = real_ms + jitter;
    if (a->timing_resolution_ms > 0) {
        double r = (double)a->timing_resolution_ms;
        result = ((int)(result / r)) * r;
    }
    return result;
}

bool eb_afp_is_font_allowed(const eb_anti_fingerprint_t *a, const char *font) {
    if (!a || !a->restrict_fonts || !font) return true;
    for (int i = 0; i < a->font_count; i++)
        if (strcasecmp(a->allowed_fonts[i], font) == 0) return true;
    return false;
}

const char *eb_afp_get_ua(const eb_anti_fingerprint_t *a) {
    if (!a || !a->spoof_ua) return NULL;
    if (a->spoofed_ua[0]) return a->spoofed_ua;
    return s_ua_pool[0];
}

void eb_afp_randomize_ua(eb_anti_fingerprint_t *a) {
    if (!a) return;
    int idx = afp_rand() % (sizeof(s_ua_pool)/sizeof(s_ua_pool[0]));
    strncpy(a->spoofed_ua, s_ua_pool[idx], EB_AFP_UA_MAX - 1);
}

void eb_afp_get_screen(const eb_anti_fingerprint_t *a, int *w, int *h, int *d) {
    if (!a || !a->normalize_screen) return;
    if (w) *w = a->reported_width;
    if (h) *h = a->reported_height;
    if (d) *d = a->reported_depth;
}

void eb_afp_calculate_score(const eb_anti_fingerprint_t *a, eb_afp_score_t *s) {
    if (!a || !s) return; memset(s, 0, sizeof(*s));
    s->canvas_entropy = a->canvas_noise ? 0.1f : 1.0f;
    s->webgl_entropy = a->webgl_spoof_vendor ? 0.1f : 1.0f;
    s->font_entropy = a->restrict_fonts ? 0.2f : 1.0f;
    s->total_entropy = (s->canvas_entropy + s->webgl_entropy + s->font_entropy) / 3.0f;
    s->unique_bits = (int)(s->total_entropy * 33);
    if (s->total_entropy < 0.3f) strcpy(s->risk, "LOW");
    else if (s->total_entropy < 0.7f) strcpy(s->risk, "MEDIUM");
    else strcpy(s->risk, "HIGH");
}

void eb_afp_destroy(eb_anti_fingerprint_t *a) { if (a) memset(a, 0, sizeof(*a)); }
