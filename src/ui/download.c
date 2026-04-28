// SPDX-License-Identifier: MIT
#include "eBrowser/download.h"
#include "eBrowser/compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static uint64_t dl_now(void) {
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec;
}

void eb_dl_init(eb_download_mgr_t *dm, const char *dir) {
    if (!dm) return; memset(dm, 0, sizeof(*dm));
    dm->next_id = 1; dm->max_concurrent = 4;
    if (dir) strncpy(dm->default_dir, dir, EB_DL_PATH_MAX-1);
    else strcpy(dm->default_dir, "/tmp/downloads");
}
void eb_dl_destroy(eb_download_mgr_t *dm) { if (dm) memset(dm, 0, sizeof(*dm)); }

static eb_download_t *find_dl(eb_download_mgr_t *dm, int id) {
    for (int i = 0; i < dm->count; i++) if (dm->downloads[i].id == id) return &dm->downloads[i];
    return NULL;
}

int eb_dl_start(eb_download_mgr_t *dm, const char *url, const char *fn) {
    if (!dm || !url || dm->count >= EB_DL_MAX) return -1;
    if (dm->active_count >= dm->max_concurrent) return -2;
    eb_download_t *d = &dm->downloads[dm->count];
    memset(d, 0, sizeof(*d));
    d->id = dm->next_id++;
    strncpy(d->url, url, EB_DL_URL_MAX-1);
    if (fn) strncpy(d->filename, fn, EB_DL_NAME_MAX-1);
    else {
        const char *last = strrchr(url, '/');
        if (last && *(last+1)) strncpy(d->filename, last+1, EB_DL_NAME_MAX-1);
        else strcpy(d->filename, "download");
    }
    snprintf(d->save_path, EB_DL_PATH_MAX, "%s/%s", dm->default_dir, d->filename);
    d->state = EB_DL_DOWNLOADING;
    d->started_at = dl_now();
    d->resumable = true;
    dm->count++; dm->active_count++;
    return d->id;
}

bool eb_dl_pause(eb_download_mgr_t *dm, int id) {
    eb_download_t *d = find_dl(dm, id);
    if (!d || d->state != EB_DL_DOWNLOADING) return false;
    d->state = EB_DL_PAUSED; dm->active_count--; return true;
}

bool eb_dl_resume(eb_download_mgr_t *dm, int id) {
    eb_download_t *d = find_dl(dm, id);
    if (!d || d->state != EB_DL_PAUSED || !d->resumable) return false;
    d->state = EB_DL_DOWNLOADING; dm->active_count++; return true;
}

bool eb_dl_cancel(eb_download_mgr_t *dm, int id) {
    eb_download_t *d = find_dl(dm, id); if (!d) return false;
    if (d->state == EB_DL_DOWNLOADING) dm->active_count--;
    d->state = EB_DL_CANCELLED; return true;
}

bool eb_dl_retry(eb_download_mgr_t *dm, int id) {
    eb_download_t *d = find_dl(dm, id);
    if (!d || d->state != EB_DL_FAILED) return false;
    d->state = EB_DL_DOWNLOADING; d->retry_count++;
    d->downloaded_bytes = 0; dm->active_count++; return true;
}

bool eb_dl_remove(eb_download_mgr_t *dm, int id) {
    if (!dm) return false;
    for (int i = 0; i < dm->count; i++)
        if (dm->downloads[i].id == id) {
            if (dm->downloads[i].state == EB_DL_DOWNLOADING) dm->active_count--;
            for (int j = i; j < dm->count-1; j++) dm->downloads[j] = dm->downloads[j+1];
            dm->count--; return true;
        }
    return false;
}

eb_download_t *eb_dl_get(eb_download_mgr_t *dm, int id) { return find_dl(dm, id); }

void eb_dl_tick(eb_download_mgr_t *dm) {
    if (!dm) return;
    for (int i = 0; i < dm->count; i++) {
        eb_download_t *d = &dm->downloads[i];
        if (d->state != EB_DL_DOWNLOADING) continue;
        /* Simulate progress - in production this reads from HTTP stream */
        if (d->total_bytes > 0) {
            d->progress_percent = (int)(d->downloaded_bytes * 100 / d->total_bytes);
            if (d->downloaded_bytes >= d->total_bytes) {
                d->state = EB_DL_COMPLETE;
                d->completed_at = dl_now();
                d->progress_percent = 100;
                dm->active_count--;
                dm->total_downloaded += d->total_bytes;
            }
        }
    }
}
