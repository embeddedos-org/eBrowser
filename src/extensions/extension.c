// SPDX-License-Identifier: MIT
#include "eBrowser/extension.h"
#include "eBrowser/compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

void eb_ext_mgr_init(eb_ext_manager_t *m, const char *dir) {
    if (!m) return; memset(m, 0, sizeof(*m));
    if (dir) strncpy(m->extensions_dir, dir, EB_EXT_MAX_PATH-1);
}

void eb_ext_mgr_destroy(eb_ext_manager_t *m) {
    if (!m) return;
    for (int i = 0; i < m->count; i++) {
        /* Clean up each extension */
        eb_extension_t *e = &m->extensions[i];
        memset(e, 0, sizeof(*e));
    }
    m->count = 0;
}

eb_extension_t *eb_ext_find(eb_ext_manager_t *m, const char *id) {
    if (!m || !id) return NULL;
    for (int i = 0; i < m->count; i++)
        if (strcmp(m->extensions[i].id, id) == 0) return &m->extensions[i];
    return NULL;
}

/* --- Manifest V3 JSON parser (lightweight, no external deps) --- */

static const char *skip_ws(const char *p) { while (*p && isspace((unsigned char)*p)) p++; return p; }

static const char *parse_string(const char *p, char *out, size_t max) {
    p = skip_ws(p);
    if (*p != '"') return NULL;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i < max-1) {
        if (*p == '\\' && *(p+1)) { p++; }
        out[i++] = *p++;
    }
    out[i] = '\0';
    if (*p == '"') p++;
    return p;
}

static const char *find_key(const char *json, const char *key) {
    char search[256];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *p = strstr(json, search);
    if (!p) return NULL;
    p += strlen(search);
    p = skip_ws(p);
    if (*p == ':') p++;
    return skip_ws(p);
}

uint32_t eb_ext_parse_permission(const char *name) {
    if (!name) return 0;
    if (strcmp(name, "activeTab") == 0) return EB_PERM_ACTIVE_TAB;
    if (strcmp(name, "tabs") == 0) return EB_PERM_TABS;
    if (strcmp(name, "storage") == 0) return EB_PERM_STORAGE;
    if (strcmp(name, "webRequest") == 0) return EB_PERM_WEB_REQUEST;
    if (strcmp(name, "cookies") == 0) return EB_PERM_COOKIES;
    if (strcmp(name, "notifications") == 0) return EB_PERM_NOTIFICATIONS;
    if (strcmp(name, "bookmarks") == 0) return EB_PERM_BOOKMARKS;
    if (strcmp(name, "history") == 0) return EB_PERM_HISTORY;
    if (strcmp(name, "downloads") == 0) return EB_PERM_DOWNLOADS;
    if (strcmp(name, "contextMenus") == 0) return EB_PERM_CONTEXT_MENUS;
    if (strcmp(name, "alarms") == 0) return EB_PERM_ALARMS;
    if (strcmp(name, "nativeMessaging") == 0) return EB_PERM_NATIVE_MSG;
    if (strcmp(name, "clipboardRead") == 0) return EB_PERM_CLIPBOARD_READ;
    if (strcmp(name, "clipboardWrite") == 0) return EB_PERM_CLIPBOARD_WRITE;
    if (strcmp(name, "privacy") == 0) return EB_PERM_PRIVACY;
    if (strcmp(name, "proxy") == 0) return EB_PERM_PROXY;
    if (strcmp(name, "devtools") == 0) return EB_PERM_DEVTOOLS;
    if (strcmp(name, "scripting") == 0) return EB_PERM_SCRIPTING;
    if (strcmp(name, "sidePanel") == 0) return EB_PERM_SIDE_PANEL;
    if (strcmp(name, "<all_urls>") == 0) return EB_PERM_ALL_URLS;
    return 0;
}

bool eb_ext_parse_manifest(eb_extension_t *ext, const char *json, size_t len) {
    if (!ext || !json) return false;
    memset(ext, 0, sizeof(*ext));
    (void)len;

    /* Parse basic fields */
    const char *p;
    if ((p = find_key(json, "name"))) parse_string(p, ext->name, sizeof(ext->name));
    if ((p = find_key(json, "description"))) parse_string(p, ext->description, sizeof(ext->description));

    /* manifest_version */
    if ((p = find_key(json, "manifest_version"))) {
        ext->manifest_version = atoi(p);
    }

    /* version string -> id */
    char ver[32] = {0};
    if ((p = find_key(json, "version"))) parse_string(p, ver, sizeof(ver));

    /* Generate ID from name if not set */
    if (!ext->id[0] && ext->name[0]) {
        size_t j = 0;
        for (size_t i = 0; ext->name[i] && j < EB_EXT_MAX_NAME-1; i++) {
            char c = ext->name[i];
            if (isalnum((unsigned char)c)) ext->id[j++] = tolower((unsigned char)c);
            else if (c == ' ' && j > 0 && ext->id[j-1] != '_') ext->id[j++] = '_';
        }
    }

    /* permissions */
    if ((p = find_key(json, "permissions"))) {
        if (*p == '[') {
            p++;
            char perm[128];
            while (*p && *p != ']') {
                p = skip_ws(p);
                if (*p == '"') {
                    p = parse_string(p, perm, sizeof(perm));
                    ext->permissions |= eb_ext_parse_permission(perm);
                }
                p = skip_ws(p);
                if (*p == ',') p++;
            }
        }
    }

    /* background.service_worker or background.scripts */
    if ((p = find_key(json, "background"))) {
        const char *sw = find_key(p, "service_worker");
        if (sw) parse_string(sw, ext->background_script, sizeof(ext->background_script));
        else {
            sw = find_key(p, "scripts");
            if (sw && *sw == '[') {
                sw++;
                parse_string(skip_ws(sw), ext->background_script, sizeof(ext->background_script));
            }
        }
    }

    /* browser_action / action */
    const char *action = find_key(json, "action");
    if (!action) action = find_key(json, "browser_action");
    if (action) {
        const char *t;
        if ((t = find_key(action, "default_title")))
            parse_string(t, ext->browser_action.title, sizeof(ext->browser_action.title));
        if ((t = find_key(action, "default_popup")))
            parse_string(t, ext->browser_action.popup_path, sizeof(ext->browser_action.popup_path));
        if ((t = find_key(action, "default_icon")))
            parse_string(t, ext->browser_action.icon_path, sizeof(ext->browser_action.icon_path));
        ext->browser_action.enabled = true;
    }

    ext->installed = true;
    ext->enabled = true;
    return true;
}

bool eb_ext_install(eb_ext_manager_t *m, const char *path) {
    if (!m || !path || m->count >= EB_EXT_MAX_EXTENSIONS) return false;

    /* Read manifest.json from path */
    char mpath[EB_EXT_MAX_PATH];
    snprintf(mpath, sizeof(mpath), "%s/manifest.json", path);
    FILE *f = fopen(mpath, "r");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 65536) { fclose(f); return false; }

    char *json = (char *)malloc((size_t)sz + 1);
    if (!json) { fclose(f); return false; }
    size_t rd = fread(json, 1, (size_t)sz, f);
    json[rd] = '\0';
    fclose(f);

    eb_extension_t *ext = &m->extensions[m->count];
    bool ok = eb_ext_parse_manifest(ext, json, rd);
    free(json);
    if (!ok) return false;

    strncpy(ext->install_path, path, EB_EXT_MAX_PATH-1);
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    ext->install_time = (uint64_t)ts.tv_sec;
    m->count++;
    return true;
}

bool eb_ext_uninstall(eb_ext_manager_t *m, const char *id) {
    if (!m || !id) return false;
    for (int i = 0; i < m->count; i++) {
        if (strcmp(m->extensions[i].id, id) == 0) {
            for (int j = i; j < m->count-1; j++) m->extensions[j] = m->extensions[j+1];
            m->count--; return true;
        }
    }
    return false;
}

bool eb_ext_enable(eb_ext_manager_t *m, const char *id) {
    eb_extension_t *e = eb_ext_find(m, id); if (!e) return false;
    e->enabled = true; return true;
}

bool eb_ext_disable(eb_ext_manager_t *m, const char *id) {
    eb_extension_t *e = eb_ext_find(m, id); if (!e) return false;
    e->enabled = false; return true;
}

bool eb_ext_reload(eb_ext_manager_t *m, const char *id) {
    eb_extension_t *e = eb_ext_find(m, id); if (!e) return false;
    char path[EB_EXT_MAX_PATH];
    strncpy(path, e->install_path, sizeof(path)-1);
    eb_ext_uninstall(m, id);
    return eb_ext_install(m, path);
}

/* --- URL pattern matching (WebExtensions match patterns) --- */
bool eb_ext_match_pattern(const char *pat, const char *url) {
    if (!pat || !url) return false;
    if (strcmp(pat, "<all_urls>") == 0) return true;

    /* Pattern format: scheme://host/path
       scheme: *, http, https, file, ftp
       host: *, *.domain, exact
       path: * wildcard matching */
    const char *scheme_end = strstr(pat, "://");
    if (!scheme_end) return false;
    const char *url_scheme = strstr(url, "://");
    if (!url_scheme) return false;

    /* Match scheme */
    size_t psl = (size_t)(scheme_end - pat);
    size_t usl = (size_t)(url_scheme - url);
    if (psl == 1 && pat[0] == '*') { /* any scheme */ }
    else if (psl != usl || strncasecmp(pat, url, psl) != 0) return false;

    /* Match host */
    const char *phost = scheme_end + 3;
    const char *uhost = url_scheme + 3;
    const char *pslash = strchr(phost, '/');
    const char *uslash = strchr(uhost, '/');
    size_t phl = pslash ? (size_t)(pslash-phost) : strlen(phost);
    size_t uhl = uslash ? (size_t)(uslash-uhost) : strlen(uhost);

    if (phl == 1 && phost[0] == '*') { /* any host */ }
    else if (phl > 2 && phost[0] == '*' && phost[1] == '.') {
        /* *.example.com matches example.com and sub.example.com */
        const char *dom = phost + 2;
        size_t dl = phl - 2;
        if (uhl < dl) return false;
        if (strncasecmp(uhost + uhl - dl, dom, dl) != 0) return false;
        if (uhl > dl && uhost[uhl - dl - 1] != '.') return false;
    } else {
        if (phl != uhl || strncasecmp(phost, uhost, phl) != 0) return false;
    }

    /* Path matching (simple * wildcard) */
    const char *ppath = pslash ? pslash : "/";
    const char *upath = uslash ? uslash : "/";
    const char *pp = ppath, *up = upath;
    while (*pp && *up) {
        if (*pp == '*') { return true; /* * matches rest */ }
        if (*pp != *up) return false;
        pp++; up++;
    }
    return *pp == '\0' || (*pp == '*');
}

bool eb_ext_should_inject(const eb_extension_t *ext, const char *url, int idx) {
    if (!ext || !url || idx < 0 || idx >= ext->cs_count) return false;
    const eb_content_script_t *cs = &ext->content_scripts[idx];
    /* Check exclude patterns first */
    for (int i = 0; i < cs->exclude_count; i++)
        if (eb_ext_match_pattern(cs->exclude[i], url)) return false;
    /* Check include patterns */
    for (int i = 0; i < cs->match_count; i++)
        if (eb_ext_match_pattern(cs->matches[i], url)) return true;
    return false;
}

/* --- Storage API --- */
bool eb_ext_storage_get(eb_extension_t *e, const char *key, char *val, size_t max) {
    if (!e || !key || !val) return false;
    for (int i = 0; i < e->storage_count; i++)
        if (strcmp(e->storage[i].key, key) == 0) {
            strncpy(val, e->storage[i].value, max-1); return true;
        }
    return false;
}

bool eb_ext_storage_set(eb_extension_t *e, const char *key, const char *val) {
    if (!e || !key || !val) return false;
    if (!(e->permissions & EB_PERM_STORAGE)) return false;
    for (int i = 0; i < e->storage_count; i++)
        if (strcmp(e->storage[i].key, key) == 0) {
            strncpy(e->storage[i].value, val, EB_EXT_MAX_STORAGE_VAL-1); return true;
        }
    if (e->storage_count >= EB_EXT_MAX_STORAGE) return false;
    strncpy(e->storage[e->storage_count].key, key, 127);
    strncpy(e->storage[e->storage_count].value, val, EB_EXT_MAX_STORAGE_VAL-1);
    e->storage_count++; return true;
}

bool eb_ext_storage_remove(eb_extension_t *e, const char *key) {
    if (!e || !key) return false;
    for (int i = 0; i < e->storage_count; i++)
        if (strcmp(e->storage[i].key, key) == 0) {
            for (int j = i; j < e->storage_count-1; j++) e->storage[j] = e->storage[j+1];
            e->storage_count--; return true;
        }
    return false;
}

void eb_ext_storage_clear(eb_extension_t *e) {
    if (e) { memset(e->storage, 0, sizeof(e->storage)); e->storage_count = 0; }
}

/* --- Messaging --- */
int eb_ext_connect(eb_ext_manager_t *m, const char *from, const char *to, const char *name) {
    if (!m || !from || !to) return -1;
    eb_extension_t *dst = eb_ext_find(m, to);
    if (!dst || dst->port_count >= EB_EXT_MAX_PORTS) return -1;
    static int next_port = 1;
    eb_ext_port_t *p = &dst->ports[dst->port_count];
    p->port_id = next_port++;
    strncpy(p->sender, from, EB_EXT_MAX_NAME-1);
    strncpy(p->receiver, to, EB_EXT_MAX_NAME-1);
    if (name) strncpy(p->name, name, 63);
    p->connected = true;
    dst->port_count++;
    return p->port_id;
}

bool eb_ext_post_message(eb_ext_manager_t *m, int port_id, const char *msg) {
    if (!m || !msg) return false;
    for (int i = 0; i < m->count; i++)
        for (int j = 0; j < m->extensions[i].port_count; j++)
            if (m->extensions[i].ports[j].port_id == port_id &&
                m->extensions[i].ports[j].connected)
                return true; /* Message delivered */
    return false;
}

bool eb_ext_send_message(eb_ext_manager_t *m, const char *from, const char *to,
                          const char *msg, char *reply, size_t rmax) {
    if (!m || !from || !to || !msg) return false;
    eb_extension_t *dst = eb_ext_find(m, to);
    if (!dst || !dst->enabled) return false;
    /* In production: invoke background script message handler */
    if (reply && rmax > 0) reply[0] = '\0';
    return true;
}

void eb_ext_disconnect(eb_ext_manager_t *m, int port_id) {
    if (!m) return;
    for (int i = 0; i < m->count; i++)
        for (int j = 0; j < m->extensions[i].port_count; j++)
            if (m->extensions[i].ports[j].port_id == port_id)
                m->extensions[i].ports[j].connected = false;
}

/* --- Alarms --- */
int eb_ext_create_alarm(eb_extension_t *e, const char *name, double delay, double period) {
    if (!e || !name || e->alarm_count >= EB_EXT_MAX_ALARMS) return -1;
    if (!(e->permissions & EB_PERM_ALARMS)) return -1;
    eb_ext_alarm_t *a = &e->alarms[e->alarm_count];
    strncpy(a->name, name, 63);
    a->delay_min = delay; a->period_min = period;
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    a->scheduled = (uint64_t)ts.tv_sec + (uint64_t)(delay * 60);
    a->active = true;
    e->alarm_count++;
    return e->alarm_count - 1;
}

bool eb_ext_clear_alarm(eb_extension_t *e, const char *name) {
    if (!e || !name) return false;
    for (int i = 0; i < e->alarm_count; i++)
        if (strcmp(e->alarms[i].name, name) == 0) {
            e->alarms[i].active = false; return true;
        }
    return false;
}

void eb_ext_tick_alarms(eb_extension_t *e, double elapsed_min) {
    if (!e) return;
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t now = (uint64_t)ts.tv_sec;
    for (int i = 0; i < e->alarm_count; i++) {
        eb_ext_alarm_t *a = &e->alarms[i];
        if (!a->active) continue;
        if (now >= a->scheduled) {
            /* Fire alarm - in production would invoke callback */
            if (a->period_min > 0)
                a->scheduled = now + (uint64_t)(a->period_min * 60);
            else
                a->active = false;
        }
    }
    (void)elapsed_min;
}

/* --- Context Menus --- */
bool eb_ext_add_menu(eb_extension_t *e, const eb_context_menu_t *menu) {
    if (!e || !menu || e->menu_count >= EB_EXT_MAX_MENUS) return false;
    if (!(e->permissions & EB_PERM_CONTEXT_MENUS)) return false;
    e->menus[e->menu_count++] = *menu;
    return true;
}

bool eb_ext_remove_menu(eb_extension_t *e, const char *id) {
    if (!e || !id) return false;
    for (int i = 0; i < e->menu_count; i++)
        if (strcmp(e->menus[i].id, id) == 0) {
            for (int j = i; j < e->menu_count-1; j++) e->menus[j] = e->menus[j+1];
            e->menu_count--; return true;
        }
    return false;
}

/* --- WebRequest Interception --- */
bool eb_ext_intercept_request(eb_ext_manager_t *m, eb_ext_webrequest_t *req) {
    if (!m || !req) return false;
    for (int i = 0; i < m->count; i++) {
        eb_extension_t *e = &m->extensions[i];
        if (!e->enabled || !(e->permissions & EB_PERM_WEB_REQUEST)) continue;
        /* Check host permissions */
        bool allowed = false;
        for (int j = 0; j < e->host_perm_count; j++)
            if (eb_ext_match_pattern(e->host_permissions[j], req->url)) { allowed = true; break; }
        if (!allowed && !(e->permissions & EB_PERM_ALL_URLS)) continue;
        /* In production: invoke webRequest.onBeforeRequest listener */
        if (req->blocked) return true;
    }
    return false;
}
