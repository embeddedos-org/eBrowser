// SPDX-License-Identifier: MIT
#include "eBrowser/bookmark.h"
#include "eBrowser/compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <strings.h>

void eb_bm_init(eb_bookmark_mgr_t *bm) {
    if (!bm) return; memset(bm, 0, sizeof(*bm));
    bm->next_id = 1;
    /* Create root folders */
    eb_bm_add_folder(bm, "Bookmarks Bar", 0);
    eb_bm_add_folder(bm, "Other Bookmarks", 0);
}
void eb_bm_destroy(eb_bookmark_mgr_t *bm) { if (bm) memset(bm, 0, sizeof(*bm)); }

static eb_bookmark_t *find_bm(eb_bookmark_mgr_t *bm, int id) {
    for (int i = 0; i < bm->count; i++) if (bm->items[i].id == id) return &bm->items[i];
    return NULL;
}

int eb_bm_add(eb_bookmark_mgr_t *bm, const char *title, const char *url, int pid) {
    if (!bm || bm->count >= EB_BM_MAX_ITEMS) return -1;
    eb_bookmark_t *b = &bm->items[bm->count];
    memset(b, 0, sizeof(*b));
    b->id = bm->next_id++; b->type = EB_BM_BOOKMARK; b->parent_id = pid;
    if (title) strncpy(b->title, title, EB_BM_TITLE_MAX-1);
    if (url) strncpy(b->url, url, EB_BM_URL_MAX-1);
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    b->created_at = (uint64_t)ts.tv_sec;
    bm->count++; return b->id;
}

int eb_bm_add_folder(eb_bookmark_mgr_t *bm, const char *title, int pid) {
    if (!bm || bm->count >= EB_BM_MAX_ITEMS) return -1;
    eb_bookmark_t *b = &bm->items[bm->count];
    memset(b, 0, sizeof(*b));
    b->id = bm->next_id++; b->type = EB_BM_FOLDER; b->parent_id = pid;
    if (title) strncpy(b->title, title, EB_BM_TITLE_MAX-1);
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    b->created_at = (uint64_t)ts.tv_sec;
    bm->count++; return b->id;
}

bool eb_bm_remove(eb_bookmark_mgr_t *bm, int id) {
    if (!bm) return false;
    for (int i = 0; i < bm->count; i++)
        if (bm->items[i].id == id) {
            for (int j = i; j < bm->count-1; j++) bm->items[j] = bm->items[j+1];
            bm->count--; return true;
        }
    return false;
}

bool eb_bm_move(eb_bookmark_mgr_t *bm, int id, int np, int pos) {
    eb_bookmark_t *b = find_bm(bm, id); if (!b) return false;
    b->parent_id = np; b->position = pos; return true;
}

bool eb_bm_update(eb_bookmark_mgr_t *bm, int id, const char *t, const char *u) {
    eb_bookmark_t *b = find_bm(bm, id); if (!b) return false;
    if (t) strncpy(b->title, t, EB_BM_TITLE_MAX-1);
    if (u) strncpy(b->url, u, EB_BM_URL_MAX-1);
    return true;
}

eb_bookmark_t *eb_bm_get(eb_bookmark_mgr_t *bm, int id) { return find_bm(bm, id); }

bool eb_bm_exists_url(const eb_bookmark_mgr_t *bm, const char *url) {
    if (!bm || !url) return false;
    for (int i = 0; i < bm->count; i++)
        if (bm->items[i].type == EB_BM_BOOKMARK && strcmp(bm->items[i].url, url) == 0) return true;
    return false;
}

int eb_bm_search(const eb_bookmark_mgr_t *bm, const char *q, int *res, int max) {
    if (!bm || !q || !res) return 0; int n = 0;
    for (int i = 0; i < bm->count && n < max; i++)
        if (strcasestr(bm->items[i].title, q) || strcasestr(bm->items[i].url, q))
            res[n++] = bm->items[i].id;
    return n;
}

int eb_bm_get_children(const eb_bookmark_mgr_t *bm, int pid, int *res, int max) {
    if (!bm || !res) return 0; int n = 0;
    for (int i = 0; i < bm->count && n < max; i++)
        if (bm->items[i].parent_id == pid) res[n++] = bm->items[i].id;
    return n;
}

bool eb_bm_add_tag(eb_bookmark_mgr_t *bm, int id, const char *tag) {
    eb_bookmark_t *b = find_bm(bm, id);
    if (!b || !tag || b->tag_count >= EB_BM_TAG_MAX) return false;
    strncpy(b->tags[b->tag_count++], tag, EB_BM_TAG_LEN-1); return true;
}

int eb_bm_save(const eb_bookmark_mgr_t *bm, const char *path) {
    if (!bm || !path) return -1;
    FILE *f = fopen(path, "w"); if (!f) return -1;
    fprintf(f, "eBrowser Bookmarks v1\n");
    for (int i = 0; i < bm->count; i++) {
        const eb_bookmark_t *b = &bm->items[i];
        fprintf(f, "%d|%d|%s|%s|%d\n", b->id, b->type, b->title, b->url, b->parent_id);
    }
    fclose(f); return bm->count;
}

int eb_bm_load(eb_bookmark_mgr_t *bm, const char *path) {
    if (!bm || !path) return -1;
    FILE *f = fopen(path, "r"); if (!f) return -1;
    char line[4096]; int n = 0;
    fgets(line, sizeof(line), f); /* skip header */
    while (fgets(line, sizeof(line), f) && bm->count < EB_BM_MAX_ITEMS) {
        eb_bookmark_t *b = &bm->items[bm->count];
        memset(b, 0, sizeof(*b));
        if (sscanf(line, "%d|%d|", &b->id, (int*)&b->type) == 2) {
            bm->count++; n++;
            if (b->id >= bm->next_id) bm->next_id = b->id + 1;
        }
    }
    fclose(f); return n;
}

int eb_bm_export_html(const eb_bookmark_mgr_t *bm, const char *path) {
    if (!bm || !path) return -1;
    FILE *f = fopen(path, "w"); if (!f) return -1;
    fprintf(f, "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">\n<TITLE>eBrowser Bookmarks</TITLE>\n<H1>Bookmarks</H1>\n<DL><p>\n");
    for (int i = 0; i < bm->count; i++) {
        const eb_bookmark_t *b = &bm->items[i];
        if (b->type == EB_BM_FOLDER) fprintf(f, "<DT><H3>%s</H3>\n<DL><p>\n", b->title);
        else if (b->type == EB_BM_BOOKMARK) fprintf(f, "<DT><A HREF=\"%s\">%s</A>\n", b->url, b->title);
    }
    fprintf(f, "</DL><p>\n"); fclose(f); return bm->count;
}

int eb_bm_import_html(eb_bookmark_mgr_t *bm, const char *path) {
    if (!bm || !path) return -1;
    /* Simplified HTML bookmark import */
    FILE *f = fopen(path, "r"); if (!f) return -1;
    char line[4096]; int n = 0;
    while (fgets(line, sizeof(line), f)) {
        char *href = strstr(line, "HREF=\"");
        if (href) {
            href += 6;
            char *end = strchr(href, '"'); if (!end) continue;
            *end = '\0';
            char *title = strchr(end+1, '>');
            if (title) {
                title++;
                char *tend = strchr(title, '<'); if (tend) *tend = '\0';
                eb_bm_add(bm, title, href, 1); n++;
            }
        }
    }
    fclose(f); return n;
}
