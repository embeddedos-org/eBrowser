// SPDX-License-Identifier: MIT
#include "eBrowser/bookmark.h"
#include "eBrowser/compat.h"
#include "eBrowser/logger.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

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

/* Strip a trailing CR/LF from a line read with fgets(). */
static char *strip_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) s[--len] = '\0';
    return s;
}

/* Parse the "id|type|" prefix of a bookmarks file line. Only the id and
 * type are read here (title/url/parent_id are PR #66's parse); a line is
 * loadable when this prefix parses with checked conversions. Rejects
 * ids that cannot be stored (<= 0, or INT_MAX, whose successor would
 * overflow next_id) and types outside the eb_bm_type_t range. */
static bool parse_bm_prefix(const char *line, int *id_out, eb_bm_type_t *type_out) {
    char *end = NULL;
    errno = 0;
    long id = strtol(line, &end, 10);
    if (errno != 0 || end == line || *end != '|') return false;
    if (id < 1 || id > INT_MAX - 1) return false; /* INT_MAX would overflow next_id */
    const char *type_start = end + 1;
    errno = 0;
    long type = strtol(type_start, &end, 10);
    if (errno != 0 || end == type_start || *end != '|') return false;
    if (type < EB_BM_BOOKMARK || type > EB_BM_SEPARATOR) return false;
    *id_out = (int)id;
    *type_out = (eb_bm_type_t)type;
    return true;
}

int eb_bm_save(const eb_bookmark_mgr_t *bm, const char *path) {
    if (!bm || !path) return -1;
    if (bm->load_errors > 0) {
        /* The manager was loaded from a damaged file; persisting it would
         * overwrite the original with a partial copy (#67). Refuse until a
         * clean load clears the flag. */
        EB_LOG_E("bookmark", "refusing to save bookmarks: manager was loaded with %d bad line(s); original file '%s' preserved",
                 bm->load_errors, path);
        return -1;
    }
    char tmp[1024], bak[1024];
    if (snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= (int)sizeof(tmp) ||
        snprintf(bak, sizeof(bak), "%s.bak", path) >= (int)sizeof(bak)) return -1;
    FILE *f = fopen(tmp, "w");
    if (!f) return -1;
    bool ok = true;
    if (fprintf(f, "eBrowser Bookmarks v1\n") < 0) ok = false;
    for (int i = 0; ok && i < bm->count; i++) {
        const eb_bookmark_t *b = &bm->items[i];
        if (fprintf(f, "%d|%d|%s|%s|%d\n", b->id, b->type, b->title, b->url, b->parent_id) < 0) ok = false;
    }
    if (fclose(f) != 0) ok = false;
    if (!ok) { remove(tmp); return -1; }
    /* Move the current file aside as the last-good backup, then install the
     * new copy. Both steps are renames, so a crash can never leave a
     * half-written bookmarks file behind, and on Windows the install works
     * because the destination no longer exists at rename time. */
    if (rename(path, bak) != 0 && errno != ENOENT) { remove(tmp); return -1; }
    if (rename(tmp, path) != 0) {
        EB_LOG_E("bookmark", "failed to install new bookmarks file '%s'; original kept as '%s'", path, bak);
        rename(bak, path); /* best effort: put the original back */
        remove(tmp);
        return -1;
    }
    return bm->count;
}

int eb_bm_load(eb_bookmark_mgr_t *bm, const char *path) {
    if (!bm || !path) return -1;
    FILE *f = fopen(path, "r");
    if (!f) {
        /* The main file may be missing because a previous save died between
         * moving it aside and installing the new copy; the last-good backup
         * still holds the bookmarks. */
        char bak[1024];
        if (snprintf(bak, sizeof(bak), "%s.bak", path) < (int)sizeof(bak)) {
            f = fopen(bak, "r");
            if (f) EB_LOG_W("bookmark", "bookmarks file '%s' missing; restored from backup '%s'", path, bak);
        }
        if (!f) return -1;
    }
    bm->load_errors = 0;
    char line[4096];
    int n = 0, errors = 0;
    /* The header identifies the file; a file without it is not a bookmarks
     * file (e.g. truncated to zero bytes) and nothing is loaded from it. */
    if (!fgets(line, sizeof(line), f) || strcmp(strip_newline(line), "eBrowser Bookmarks v1") != 0) {
        fclose(f);
        bm->load_errors = 1;
        EB_LOG_E("bookmark", "bookmarks file '%s' has a missing or bad header; keeping in-memory bookmarks", path);
        return -2;
    }
    /* Ids already read from this file. A second occurrence is corruption
     * (the #65 saver wrote duplicate root lines); it is skipped, never
     * allowed to replace the good record (#67, and the #66 hold note). */
    int seen[EB_BM_MAX_ITEMS]; int seen_count = 0;
    int lineno = 1;
    while (fgets(line, sizeof(line), f)) {
        lineno++;
        size_t len = strlen(line);
        bool truncated = len > 0 && line[len - 1] != '\n';
        int id; eb_bm_type_t type;
        if (truncated || !parse_bm_prefix(line, &id, &type)) {
            errors++;
            EB_LOG_W("bookmark", "skipping malformed bookmarks line %d in '%s'%s",
                     lineno, path, truncated ? " (truncated file)" : "");
            continue;
        }
        bool dup = false;
        for (int i = 0; i < seen_count; i++) if (seen[i] == id) { dup = true; break; }
        if (dup) {
            errors++;
            EB_LOG_W("bookmark", "skipping duplicate bookmarks id %d (line %d) in '%s'", id, lineno, path);
            continue;
        }
        /* The seen-set is capped at EB_BM_MAX_ITEMS: a file with more
         * well-formed lines than the manager can hold errors out on the
         * capacity check below anyway, so nothing is lost. */
        if (seen_count < EB_BM_MAX_ITEMS) seen[seen_count++] = id;
        /* An id already in the manager is one of the root folders
         * eb_bm_init() created; the file's copy of it carries no new data
         * (titles are loaded by #66), so keep the in-memory record. */
        if (find_bm(bm, id)) {
            n++;
            if (id >= bm->next_id) bm->next_id = id + 1;
            continue;
        }
        if (bm->count >= EB_BM_MAX_ITEMS) {
            errors++;
            EB_LOG_W("bookmark", "bookmarks file '%s' exceeds %d items; skipping id %d (line %d)",
                     path, EB_BM_MAX_ITEMS, id, lineno);
            continue;
        }
        eb_bookmark_t *b = &bm->items[bm->count];
        memset(b, 0, sizeof(*b));
        b->id = id; b->type = type;
        bm->count++; n++;
        if (id >= bm->next_id) bm->next_id = id + 1;
    }
    if (ferror(f)) {
        errors++;
        EB_LOG_E("bookmark", "error while reading bookmarks file '%s'", path);
    }
    fclose(f);
    bm->load_errors = errors;
    if (errors > 0) {
        EB_LOG_W("bookmark", "bookmarks file '%s' loaded with %d bad line(s); %d record(s) kept, original file preserved",
                 path, errors, n);
        return -2;
    }
    return n;
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
