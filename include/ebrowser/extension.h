// SPDX-License-Identifier: MIT
#ifndef eBrowser_EXTENSION_H
#define eBrowser_EXTENSION_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define EB_EXT_MAX_EXTENSIONS     64
#define EB_EXT_MAX_NAME           128
#define EB_EXT_MAX_CS             16
#define EB_EXT_MAX_MATCHES        32
#define EB_EXT_MAX_MATCH_LEN      256
#define EB_EXT_MAX_PATH           512
#define EB_EXT_MAX_STORAGE        256
#define EB_EXT_MAX_STORAGE_VAL    4096
#define EB_EXT_MAX_PORTS          32
#define EB_EXT_MAX_MENUS          64
#define EB_EXT_MAX_ALARMS         32

typedef enum {
    EB_PERM_NONE=0, EB_PERM_ACTIVE_TAB=(1<<0), EB_PERM_TABS=(1<<1),
    EB_PERM_STORAGE=(1<<2), EB_PERM_WEB_REQUEST=(1<<3), EB_PERM_COOKIES=(1<<4),
    EB_PERM_NOTIFICATIONS=(1<<5), EB_PERM_BOOKMARKS=(1<<6), EB_PERM_HISTORY=(1<<7),
    EB_PERM_DOWNLOADS=(1<<8), EB_PERM_CONTEXT_MENUS=(1<<9), EB_PERM_ALARMS=(1<<10),
    EB_PERM_NATIVE_MSG=(1<<11), EB_PERM_CLIPBOARD_READ=(1<<12),
    EB_PERM_CLIPBOARD_WRITE=(1<<13), EB_PERM_ALL_URLS=(1<<15),
    EB_PERM_PRIVACY=(1<<17), EB_PERM_PROXY=(1<<18), EB_PERM_DEVTOOLS=(1<<19),
    EB_PERM_SCRIPTING=(1<<21), EB_PERM_SIDE_PANEL=(1<<22)
} eb_ext_perm_t;

typedef enum { EB_CS_DOCUMENT_START, EB_CS_DOCUMENT_END, EB_CS_DOCUMENT_IDLE } eb_cs_run_at_t;

typedef struct {
    char matches[EB_EXT_MAX_MATCHES][EB_EXT_MAX_MATCH_LEN]; int match_count;
    char exclude[EB_EXT_MAX_MATCHES][EB_EXT_MAX_MATCH_LEN]; int exclude_count;
    char js_files[8][EB_EXT_MAX_PATH]; int js_count;
    char css_files[8][EB_EXT_MAX_PATH]; int css_count;
    eb_cs_run_at_t run_at; bool all_frames;
} eb_content_script_t;

typedef struct {
    char title[128]; char icon_path[EB_EXT_MAX_PATH]; char popup_path[EB_EXT_MAX_PATH];
    char badge_text[32]; int badge_bg_color; bool enabled;
} eb_browser_action_t;

typedef struct { char id[64]; char title[256]; char parent_id[64]; int contexts; bool enabled; } eb_context_menu_t;
typedef struct { char name[64]; double delay_min, period_min; uint64_t scheduled; bool active; } eb_ext_alarm_t;
typedef struct { char key[128]; char value[EB_EXT_MAX_STORAGE_VAL]; } eb_ext_storage_entry_t;
typedef struct { char name[64]; char sender[EB_EXT_MAX_NAME]; char receiver[EB_EXT_MAX_NAME]; bool connected; int port_id; } eb_ext_port_t;

typedef struct {
    char id[EB_EXT_MAX_NAME], name[EB_EXT_MAX_NAME], description[512];
    int manifest_version; uint32_t permissions, optional_permissions;
    char host_permissions[EB_EXT_MAX_MATCHES][EB_EXT_MAX_MATCH_LEN]; int host_perm_count;
    eb_content_script_t content_scripts[EB_EXT_MAX_CS]; int cs_count;
    char background_script[EB_EXT_MAX_PATH]; bool background_persistent;
    eb_browser_action_t browser_action;
    eb_context_menu_t menus[EB_EXT_MAX_MENUS]; int menu_count;
    eb_ext_alarm_t alarms[EB_EXT_MAX_ALARMS]; int alarm_count;
    eb_ext_storage_entry_t storage[EB_EXT_MAX_STORAGE]; int storage_count;
    eb_ext_port_t ports[EB_EXT_MAX_PORTS]; int port_count;
    char install_path[EB_EXT_MAX_PATH];
    bool enabled, installed, dev_mode;
    uint64_t install_time, update_time; size_t memory_usage;
} eb_extension_t;

typedef struct {
    eb_extension_t extensions[EB_EXT_MAX_EXTENSIONS]; int count;
    char extensions_dir[EB_EXT_MAX_PATH]; bool dev_mode_global;
} eb_ext_manager_t;

void eb_ext_mgr_init(eb_ext_manager_t *mgr, const char *ext_dir);
void eb_ext_mgr_destroy(eb_ext_manager_t *mgr);
bool eb_ext_install(eb_ext_manager_t *mgr, const char *path);
bool eb_ext_uninstall(eb_ext_manager_t *mgr, const char *id);
bool eb_ext_enable(eb_ext_manager_t *mgr, const char *id);
bool eb_ext_disable(eb_ext_manager_t *mgr, const char *id);
bool eb_ext_reload(eb_ext_manager_t *mgr, const char *id);
eb_extension_t *eb_ext_find(eb_ext_manager_t *mgr, const char *id);
bool eb_ext_parse_manifest(eb_extension_t *ext, const char *json, size_t len);
uint32_t eb_ext_parse_permission(const char *name);
bool eb_ext_should_inject(const eb_extension_t *ext, const char *url, int cs_idx);
bool eb_ext_match_pattern(const char *pattern, const char *url);
bool eb_ext_storage_get(eb_extension_t *ext, const char *key, char *value, size_t max);
bool eb_ext_storage_set(eb_extension_t *ext, const char *key, const char *value);
bool eb_ext_storage_remove(eb_extension_t *ext, const char *key);
void eb_ext_storage_clear(eb_extension_t *ext);
int  eb_ext_connect(eb_ext_manager_t *mgr, const char *from, const char *to, const char *name);
bool eb_ext_post_message(eb_ext_manager_t *mgr, int port_id, const char *msg);
bool eb_ext_send_message(eb_ext_manager_t *mgr, const char *from, const char *to, const char *msg, char *reply, size_t reply_max);
void eb_ext_disconnect(eb_ext_manager_t *mgr, int port_id);
int  eb_ext_create_alarm(eb_extension_t *ext, const char *name, double delay, double period);
bool eb_ext_clear_alarm(eb_extension_t *ext, const char *name);
void eb_ext_tick_alarms(eb_extension_t *ext, double elapsed_min);
bool eb_ext_add_menu(eb_extension_t *ext, const eb_context_menu_t *menu);
bool eb_ext_remove_menu(eb_extension_t *ext, const char *id);

typedef struct {
    const char *url, *method, *type; int tab_id;
    bool blocked, redirected; char redirect_url[EB_EXT_MAX_MATCH_LEN];
} eb_ext_webrequest_t;
bool eb_ext_intercept_request(eb_ext_manager_t *mgr, eb_ext_webrequest_t *req);
#endif
