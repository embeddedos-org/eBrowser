// SPDX-License-Identifier: MIT
#ifndef eBrowser_DEVTOOLS_H
#define eBrowser_DEVTOOLS_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "eBrowser/dom.h"

#define EB_DT_MAX_CONSOLE 512
#define EB_DT_MAX_NETWORK 256
#define EB_DT_MAX_MSG     1024

typedef enum { EB_DT_LOG, EB_DT_WARN, EB_DT_ERROR, EB_DT_INFO, EB_DT_DEBUG } eb_dt_level_t;

typedef struct { eb_dt_level_t level; char message[EB_DT_MAX_MSG]; uint64_t timestamp; char source[256]; int line; } eb_dt_console_entry_t;

typedef struct {
    char url[1024]; char method[16]; int status; char content_type[128];
    size_t req_size, res_size; uint64_t start_time, end_time, duration_ms;
    bool from_cache, blocked; char initiator[256];
} eb_dt_network_entry_t;

typedef struct {
    eb_dt_console_entry_t console[EB_DT_MAX_CONSOLE]; int console_count;
    eb_dt_network_entry_t network[EB_DT_MAX_NETWORK]; int network_count;
    eb_dom_node_t *inspected_node; bool dom_overlay;
    bool open; int active_panel; bool preserve_log;
} eb_devtools_t;

void  eb_dt_init(eb_devtools_t *dt);
void  eb_dt_destroy(eb_devtools_t *dt);
void  eb_dt_toggle(eb_devtools_t *dt);
void  eb_dt_log(eb_devtools_t *dt, eb_dt_level_t level, const char *src, int line, const char *fmt, ...);
void  eb_dt_clear_console(eb_devtools_t *dt);
void  eb_dt_record_request(eb_devtools_t *dt, const eb_dt_network_entry_t *entry);
void  eb_dt_clear_network(eb_devtools_t *dt);
void  eb_dt_inspect_node(eb_devtools_t *dt, eb_dom_node_t *node);
#endif
