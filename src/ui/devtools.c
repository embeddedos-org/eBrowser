// SPDX-License-Identifier: MIT
#include "eBrowser/devtools.h"
#include "eBrowser/compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

void eb_dt_init(eb_devtools_t *dt) { if (dt) memset(dt, 0, sizeof(*dt)); }
void eb_dt_destroy(eb_devtools_t *dt) { if (dt) memset(dt, 0, sizeof(*dt)); }

void eb_dt_toggle(eb_devtools_t *dt) { if (dt) dt->open = !dt->open; }

void eb_dt_log(eb_devtools_t *dt, eb_dt_level_t lv, const char *src, int line, const char *fmt, ...) {
    if (!dt || dt->console_count >= EB_DT_MAX_CONSOLE) return;
    eb_dt_console_entry_t *e = &dt->console[dt->console_count];
    memset(e, 0, sizeof(*e));
    e->level = lv; e->line = line;
    if (src) strncpy(e->source, src, sizeof(e->source)-1);
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    e->timestamp = (uint64_t)ts.tv_sec;
    va_list ap; va_start(ap, fmt);
    vsnprintf(e->message, EB_DT_MAX_MSG, fmt, ap);
    va_end(ap);
    dt->console_count++;
}

void eb_dt_clear_console(eb_devtools_t *dt) {
    if (dt && !dt->preserve_log) { memset(dt->console, 0, sizeof(dt->console)); dt->console_count = 0; }
}

void eb_dt_record_request(eb_devtools_t *dt, const eb_dt_network_entry_t *e) {
    if (!dt || !e || dt->network_count >= EB_DT_MAX_NETWORK) return;
    dt->network[dt->network_count++] = *e;
}

void eb_dt_clear_network(eb_devtools_t *dt) {
    if (dt) { memset(dt->network, 0, sizeof(dt->network)); dt->network_count = 0; }
}

void eb_dt_inspect_node(eb_devtools_t *dt, eb_dom_node_t *node) {
    if (dt) { dt->inspected_node = node; dt->dom_overlay = (node != NULL); }
}
