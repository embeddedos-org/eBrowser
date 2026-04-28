// SPDX-License-Identifier: MIT
#ifndef eBrowser_ANTI_FINGERPRINT_H
#define eBrowser_ANTI_FINGERPRINT_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define EB_AFP_MAX_FONTS    64
#define EB_AFP_UA_MAX       512

typedef enum { EB_AFP_OFF, EB_AFP_STANDARD, EB_AFP_STRICT, EB_AFP_TOR_LIKE } eb_afp_level_t;

typedef struct {
    bool canvas_noise; int canvas_noise_bits; bool canvas_readback_block;
    bool webgl_spoof_vendor, webgl_spoof_renderer;
    char webgl_vendor[128], webgl_renderer[128]; bool webgl_block_extensions;
    bool restrict_fonts; char allowed_fonts[EB_AFP_MAX_FONTS][128]; int font_count;
    bool jitter_timing; int timing_jitter_us, timing_resolution_ms;
    bool normalize_screen; int reported_width, reported_height, reported_depth;
    bool spoof_ua; char spoofed_ua[EB_AFP_UA_MAX]; bool randomize_ua_per_session;
    bool spoof_cores; int reported_cores; bool spoof_memory; int reported_memory_gb;
    bool block_webrtc_ip, block_battery_api, block_bluetooth_api, block_usb_api;
    bool audio_noise; float audio_noise_level;
    eb_afp_level_t level;
} eb_anti_fingerprint_t;

void  eb_afp_init(eb_anti_fingerprint_t *afp);
void  eb_afp_set_level(eb_anti_fingerprint_t *afp, eb_afp_level_t level);
void  eb_afp_destroy(eb_anti_fingerprint_t *afp);
void  eb_afp_inject_canvas_noise(eb_anti_fingerprint_t *afp, uint8_t *pixels, int w, int h, int ch);
bool  eb_afp_should_block_canvas_read(const eb_anti_fingerprint_t *afp);
const char *eb_afp_get_webgl_vendor(const eb_anti_fingerprint_t *afp);
const char *eb_afp_get_webgl_renderer(const eb_anti_fingerprint_t *afp);
double eb_afp_jitter_time(const eb_anti_fingerprint_t *afp, double real_ms);
bool  eb_afp_is_font_allowed(const eb_anti_fingerprint_t *afp, const char *font);
const char *eb_afp_get_ua(const eb_anti_fingerprint_t *afp);
void  eb_afp_randomize_ua(eb_anti_fingerprint_t *afp);
void  eb_afp_get_screen(const eb_anti_fingerprint_t *afp, int *w, int *h, int *depth);
typedef struct { float canvas_entropy, webgl_entropy, font_entropy, total_entropy; int unique_bits; char risk[32]; } eb_afp_score_t;
void  eb_afp_calculate_score(const eb_anti_fingerprint_t *afp, eb_afp_score_t *score);
#endif
