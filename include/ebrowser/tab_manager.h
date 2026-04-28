// SPDX-License-Identifier: MIT
#ifndef eBrowser_TAB_MANAGER_H
#define eBrowser_TAB_MANAGER_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define EB_TAB_MAX          64
#define EB_TAB_MAX_GROUPS   16
#define EB_TAB_MAX_CLOSED   32
#define EB_TAB_URL_MAX      2048
#define EB_TAB_TITLE_MAX    256

typedef enum { EB_TAB_LOADING, EB_TAB_COMPLETE, EB_TAB_ERROR, EB_TAB_HIBERNATED, EB_TAB_CRASHED } eb_tab_state_t;

typedef struct {
    int id; char url[EB_TAB_URL_MAX]; char title[EB_TAB_TITLE_MAX]; eb_tab_state_t state;
    int group_id; bool pinned, muted, hibernated, incognito;
    int zoom_percent, scroll_y; size_t memory_usage;
    uint64_t created_at, last_accessed; int parent_tab_id;
} eb_managed_tab_t;

typedef struct { int id; char name[64]; int color; bool collapsed; int tab_ids[EB_TAB_MAX]; int tab_count; } eb_tab_group_t;
typedef struct { char url[EB_TAB_URL_MAX]; char title[EB_TAB_TITLE_MAX]; uint64_t closed_at; int group_id; } eb_closed_tab_t;

typedef struct {
    eb_managed_tab_t tabs[EB_TAB_MAX]; int tab_count, active_tab, next_tab_id;
    eb_tab_group_t groups[EB_TAB_MAX_GROUPS]; int group_count, next_group_id;
    eb_closed_tab_t closed[EB_TAB_MAX_CLOSED]; int closed_count;
    int split_left, split_right; bool split_active;
    size_t total_memory, memory_limit;
} eb_tab_manager_t;

void  eb_tabmgr_init(eb_tab_manager_t *tm);
void  eb_tabmgr_destroy(eb_tab_manager_t *tm);
int   eb_tabmgr_new(eb_tab_manager_t *tm, const char *url, bool incognito);
void  eb_tabmgr_close(eb_tab_manager_t *tm, int tab_id);
void  eb_tabmgr_switch(eb_tab_manager_t *tm, int tab_id);
int   eb_tabmgr_duplicate(eb_tab_manager_t *tm, int tab_id);
void  eb_tabmgr_pin(eb_tab_manager_t *tm, int tab_id, bool pin);
void  eb_tabmgr_mute(eb_tab_manager_t *tm, int tab_id, bool mute);
void  eb_tabmgr_set_zoom(eb_tab_manager_t *tm, int tab_id, int percent);
eb_managed_tab_t *eb_tabmgr_get(eb_tab_manager_t *tm, int tab_id);
int   eb_tabmgr_create_group(eb_tab_manager_t *tm, const char *name, int color);
bool  eb_tabmgr_add_to_group(eb_tab_manager_t *tm, int tab_id, int group_id);
void  eb_tabmgr_collapse_group(eb_tab_manager_t *tm, int group_id, bool collapse);
void  eb_tabmgr_hibernate(eb_tab_manager_t *tm, int tab_id);
void  eb_tabmgr_wake(eb_tab_manager_t *tm, int tab_id);
void  eb_tabmgr_auto_hibernate(eb_tab_manager_t *tm, int inactive_sec);
void  eb_tabmgr_split(eb_tab_manager_t *tm, int left_id, int right_id);
void  eb_tabmgr_unsplit(eb_tab_manager_t *tm);
int   eb_tabmgr_reopen_closed(eb_tab_manager_t *tm);
int   eb_tabmgr_search(const eb_tab_manager_t *tm, const char *query, int *results, int max);
#endif
