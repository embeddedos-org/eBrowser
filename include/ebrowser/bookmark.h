// SPDX-License-Identifier: MIT
#ifndef eBrowser_BOOKMARK_H
#define eBrowser_BOOKMARK_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define EB_BM_MAX_ITEMS   1024
#define EB_BM_URL_MAX     2048
#define EB_BM_TITLE_MAX   256
#define EB_BM_TAG_MAX     16
#define EB_BM_TAG_LEN     64

typedef enum { EB_BM_BOOKMARK, EB_BM_FOLDER, EB_BM_SEPARATOR } eb_bm_type_t;

typedef struct {
    int id; eb_bm_type_t type; char title[EB_BM_TITLE_MAX]; char url[EB_BM_URL_MAX];
    int parent_id, position; char tags[EB_BM_TAG_MAX][EB_BM_TAG_LEN]; int tag_count;
    uint64_t created_at, last_visited; int visit_count;
} eb_bookmark_t;

typedef struct { eb_bookmark_t items[EB_BM_MAX_ITEMS]; int count, next_id; char file_path[512]; } eb_bookmark_mgr_t;

void  eb_bm_init(eb_bookmark_mgr_t *bm);
void  eb_bm_destroy(eb_bookmark_mgr_t *bm);
int   eb_bm_add(eb_bookmark_mgr_t *bm, const char *title, const char *url, int parent_id);
int   eb_bm_add_folder(eb_bookmark_mgr_t *bm, const char *title, int parent_id);
bool  eb_bm_remove(eb_bookmark_mgr_t *bm, int id);
bool  eb_bm_move(eb_bookmark_mgr_t *bm, int id, int new_parent, int pos);
bool  eb_bm_update(eb_bookmark_mgr_t *bm, int id, const char *title, const char *url);
eb_bookmark_t *eb_bm_get(eb_bookmark_mgr_t *bm, int id);
bool  eb_bm_exists_url(const eb_bookmark_mgr_t *bm, const char *url);
int   eb_bm_search(const eb_bookmark_mgr_t *bm, const char *query, int *results, int max);
int   eb_bm_get_children(const eb_bookmark_mgr_t *bm, int parent_id, int *results, int max);
bool  eb_bm_add_tag(eb_bookmark_mgr_t *bm, int id, const char *tag);
int   eb_bm_save(const eb_bookmark_mgr_t *bm, const char *path);
int   eb_bm_load(eb_bookmark_mgr_t *bm, const char *path);
int   eb_bm_export_html(const eb_bookmark_mgr_t *bm, const char *path);
int   eb_bm_import_html(eb_bookmark_mgr_t *bm, const char *path);
#endif
