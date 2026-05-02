// SPDX-License-Identifier: MIT
// eBrowser SDL2 Desktop Entry Point — real GUI window with LVGL rendering.
//
// Provides:
//   - Mouse + keyboard input pipelines (SDL_TEXTINPUT + SDL_KEYDOWN -> LVGL keypad indev)
//   - LVGL default group with the URL bar focused on launch (Chrome-like)
//   - Default 1280x800 Chrome-style window (override via EBROWSER_WIDTH / EBROWSER_HEIGHT)
//   - Optional dark mode via EBROWSER_DARK=1
//   - Version banner + window title sourced from CMake (EB_VERSION)
//   - Quit hook from in-app menu (browser->quit_requested)
//
#include "lvgl.h"
#include "eBrowser/browser.h"
#include "eb_version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>

/* ---------- Window sizing ---------- */
/* Maximum framebuffer we will ever allocate statically (avoids malloc). */
#define EB_MAX_W 1920
#define EB_MAX_H 1200
#define EB_DEFAULT_W 1280
#define EB_DEFAULT_H 800
#define EB_MIN_W 640
#define EB_MIN_H 400

static int s_w = EB_DEFAULT_W;
static int s_h = EB_DEFAULT_H;

/* SDL + LVGL globals */
static SDL_Window   *s_window   = NULL;
static SDL_Renderer *s_renderer = NULL;
static SDL_Texture  *s_texture  = NULL;
static lv_display_t *s_display  = NULL;
static lv_indev_t   *s_mouse    = NULL;
static lv_indev_t   *s_keyboard = NULL;
static lv_group_t   *s_kb_group = NULL;
static eb_browser_t  s_browser;
static volatile int   s_running = 1;

/* Static framebuffers sized for the maximum window we ever support. */
static uint8_t s_fb[EB_MAX_W * EB_MAX_H * 4];
static uint8_t s_lv_buf[EB_MAX_W * 40 * 2]; /* LVGL partial render buffer (RGB565) */

static int s_mouse_pressed = 0;
static int s_mouse_x = 0, s_mouse_y = 0;

/* ---------- Keyboard ring buffer ----------
 * SDL events fill the buffer; LVGL's keypad read_cb drains it one key at a time.
 */
#define EB_KEY_BUF_SIZE 64
typedef struct {
    uint32_t key;
    uint8_t  pressed; /* 1 = press, 0 = release */
} eb_key_event_t;

static eb_key_event_t s_key_buf[EB_KEY_BUF_SIZE];
static int s_key_head = 0; /* write idx */
static int s_key_tail = 0; /* read  idx */

static void key_buf_push(uint32_t key, int pressed) {
    int next = (s_key_head + 1) % EB_KEY_BUF_SIZE;
    if (next == s_key_tail) return; /* full — drop */
    s_key_buf[s_key_head].key = key;
    s_key_buf[s_key_head].pressed = pressed ? 1 : 0;
    s_key_head = next;
}

static int key_buf_pop(eb_key_event_t *out) {
    if (s_key_tail == s_key_head) return 0;
    *out = s_key_buf[s_key_tail];
    s_key_tail = (s_key_tail + 1) % EB_KEY_BUF_SIZE;
    return 1;
}

static int key_buf_has_more(void) {
    return s_key_tail != s_key_head;
}

/* SDLK_* -> LV_KEY_* translation for non-text keys. Returns 0 if not mapped. */
static uint32_t sdl_key_to_lv(SDL_Keycode k) {
    switch (k) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER:  return LV_KEY_ENTER;
        case SDLK_BACKSPACE: return LV_KEY_BACKSPACE;
        case SDLK_DELETE:    return LV_KEY_DEL;
        case SDLK_ESCAPE:    return LV_KEY_ESC;
        case SDLK_LEFT:      return LV_KEY_LEFT;
        case SDLK_RIGHT:     return LV_KEY_RIGHT;
        case SDLK_UP:        return LV_KEY_UP;
        case SDLK_DOWN:      return LV_KEY_DOWN;
        case SDLK_HOME:      return LV_KEY_HOME;
        case SDLK_END:       return LV_KEY_END;
        case SDLK_TAB:       return LV_KEY_NEXT;
        default:             return 0;
    }
}

/* ---------- LVGL callbacks ---------- */
static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    int32_t x, y;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            int src_idx = (y * w + x) * 2;
            uint16_t pixel = (uint16_t)((uint16_t)px_map[src_idx] | ((uint16_t)px_map[src_idx + 1] << 8));
            uint8_t r = (uint8_t)(((pixel >> 11) & 0x1F) << 3);
            uint8_t g = (uint8_t)(((pixel >> 5) & 0x3F) << 2);
            uint8_t b = (uint8_t)((pixel & 0x1F) << 3);

            int dst_idx = ((area->y1 + y) * s_w + (area->x1 + x)) * 4;
            s_fb[dst_idx + 0] = b;
            s_fb[dst_idx + 1] = g;
            s_fb[dst_idx + 2] = r;
            s_fb[dst_idx + 3] = 255;
        }
    }

    lv_display_flush_ready(disp);
}

static void mouse_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    (void)indev;
    data->point.x = s_mouse_x;
    data->point.y = s_mouse_y;
    data->state = s_mouse_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void keyboard_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    (void)indev;
    eb_key_event_t ev;
    if (key_buf_pop(&ev)) {
        data->key = ev.key;
        data->state = ev.pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    data->continue_reading = key_buf_has_more();
}

static uint32_t tick_cb(void) {
    return SDL_GetTicks();
}

/* ---------- Helpers ---------- */
static int env_int(const char *name, int dflt, int min_val, int max_val) {
    const char *v = getenv(name);
    if (!v || !*v) return dflt;
    char *end = NULL;
    long n = strtol(v, &end, 10);
    if (end == v || n < (long)min_val || n > (long)max_val) return dflt;
    return (int)n;
}

static int env_bool(const char *name) {
    const char *v = getenv(name);
    if (!v || !*v) return 0;
    return (v[0] == '1' || v[0] == 'y' || v[0] == 'Y' || v[0] == 't' || v[0] == 'T');
}

/* Push a UTF-8 byte stream into the key ring. ASCII codepoints pass through;
 * UTF-8 multi-byte sequences are also forwarded as their raw codepoint
 * (good enough for URL entry, which is ASCII anyway). */
static void push_text_input(const char *utf8) {
    while (*utf8) {
        unsigned char c = (unsigned char)*utf8++;
        uint32_t cp;
        if (c < 0x80) {
            cp = c;
        } else if ((c & 0xE0) == 0xC0 && *utf8) {
            cp = ((uint32_t)(c & 0x1F) << 6) | ((uint32_t)((unsigned char)*utf8 & 0x3F));
            utf8 += 1;
        } else if ((c & 0xF0) == 0xE0 && utf8[0] && utf8[1]) {
            cp = ((uint32_t)(c & 0x0F) << 12)
               | ((uint32_t)((unsigned char)utf8[0] & 0x3F) << 6)
               | ((uint32_t)((unsigned char)utf8[1] & 0x3F));
            utf8 += 2;
        } else if ((c & 0xF8) == 0xF0 && utf8[0] && utf8[1] && utf8[2]) {
            cp = ((uint32_t)(c & 0x07) << 18)
               | ((uint32_t)((unsigned char)utf8[0] & 0x3F) << 12)
               | ((uint32_t)((unsigned char)utf8[1] & 0x3F) << 6)
               | ((uint32_t)((unsigned char)utf8[2] & 0x3F));
            utf8 += 3;
        } else {
            continue; /* malformed — skip */
        }
        /* Send as press immediately followed by release so LVGL textareas
         * register one character per keystroke. */
        key_buf_push(cp, 1);
        key_buf_push(cp, 0);
    }
}

int main(int argc, char *argv[]) {
    /* Honour environment overrides for window size + theme. */
    s_w = env_int("EBROWSER_WIDTH",  EB_DEFAULT_W, EB_MIN_W, EB_MAX_W);
    s_h = env_int("EBROWSER_HEIGHT", EB_DEFAULT_H, EB_MIN_H, EB_MAX_H);
    int dark = env_bool("EBROWSER_DARK");

    printf("[eBrowser] Starting eBrowser v%s (window %dx%d%s)...\n",
           EB_VERSION, s_w, s_h, dark ? ", dark" : "");

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "[eBrowser] SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    char title[64];
    snprintf(title, sizeof(title), "eBrowser %s", EB_VERSION);

    s_window = SDL_CreateWindow(title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        s_w, s_h, SDL_WINDOW_SHOWN);
    if (!s_window) {
        fprintf(stderr, "[eBrowser] SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    s_renderer = SDL_CreateRenderer(s_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!s_renderer) {
        s_renderer = SDL_CreateRenderer(s_window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!s_renderer) {
        fprintf(stderr, "[eBrowser] SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(s_window);
        SDL_Quit();
        return 1;
    }

    s_texture = SDL_CreateTexture(s_renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        s_w, s_h);

    /* Enable text input so SDL emits SDL_TEXTINPUT events (Chrome-like typing). */
    SDL_StartTextInput();

    printf("[eBrowser] Initializing LVGL...\n");
    lv_init();
    lv_tick_set_cb(tick_cb);

    s_display = lv_display_create(s_w, s_h);
    lv_display_set_buffers(s_display, s_lv_buf, NULL, sizeof(s_lv_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(s_display, flush_cb);

    /* Apply Chrome-like theme — light by default, dark via EBROWSER_DARK=1. */
    lv_color_t primary   = lv_color_make(0x1A, 0x73, 0xE8); /* Chrome blue */
    lv_color_t secondary = lv_color_make(0x5F, 0x63, 0x68); /* Chrome grey  */
    lv_theme_t *theme = lv_theme_default_init(s_display, primary, secondary,
                                              dark ? true : false,
                                              &lv_font_montserrat_14);
    if (theme) lv_display_set_theme(s_display, theme);

    /* Pointer indev */
    s_mouse = lv_indev_create();
    lv_indev_set_type(s_mouse, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_mouse, mouse_read_cb);

    /* Keyboard indev + default group (LVGL routes keys to the focused obj here) */
    s_kb_group = lv_group_create();
    lv_group_set_default(s_kb_group);

    s_keyboard = lv_indev_create();
    lv_indev_set_type(s_keyboard, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_keyboard, keyboard_read_cb);
    lv_indev_set_group(s_keyboard, s_kb_group);

    printf("[eBrowser] Creating browser UI...\n");
    lv_obj_t *scr = lv_screen_active();
    eb_browser_init(&s_browser, scr);

    /* Focus the URL bar so the user can immediately type, just like Chrome's omnibox. */
    if (s_browser.url_bar) {
        lv_group_add_obj(s_kb_group, s_browser.url_bar);
        lv_group_focus_obj(s_browser.url_bar);
    }

    if (argc > 1) {
        printf("[eBrowser] Navigating to %s\n", argv[1]);
        eb_browser_navigate(&s_browser, argv[1]);
    }

    printf("[eBrowser] Browser ready — %dx%d window\n", s_w, s_h);

    while (s_running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:
                    s_running = 0;
                    break;
                case SDL_MOUSEMOTION:
                    s_mouse_x = ev.motion.x;
                    s_mouse_y = ev.motion.y;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    s_mouse_pressed = 1;
                    s_mouse_x = ev.button.x;
                    s_mouse_y = ev.button.y;
                    break;
                case SDL_MOUSEBUTTONUP:
                    s_mouse_pressed = 0;
                    break;
                case SDL_TEXTINPUT:
                    push_text_input(ev.text.text);
                    break;
                case SDL_KEYDOWN: {
                    uint32_t k = sdl_key_to_lv(ev.key.keysym.sym);
                    if (k) {
                        /* Send press + release so LVGL textarea consumes it once. */
                        key_buf_push(k, 1);
                        key_buf_push(k, 0);
                    }
                    break;
                }
                default:
                    break;
            }
        }

        lv_timer_handler();

        SDL_UpdateTexture(s_texture, NULL, s_fb, s_w * 4);
        SDL_RenderClear(s_renderer);
        SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
        SDL_RenderPresent(s_renderer);

        SDL_Delay(5);

        /* Honour an in-app Quit menu request. */
        if (s_browser.quit_requested) s_running = 0;
    }

    printf("[eBrowser] Shutting down...\n");
    eb_browser_deinit(&s_browser);
    lv_deinit();

    SDL_StopTextInput();
    SDL_DestroyTexture(s_texture);
    SDL_DestroyRenderer(s_renderer);
    SDL_DestroyWindow(s_window);
    SDL_Quit();

    return 0;
}
