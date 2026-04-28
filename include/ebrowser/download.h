// SPDX-License-Identifier: MIT
#ifndef eBrowser_DOWNLOAD_H
#define eBrowser_DOWNLOAD_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define EB_DL_MAX         64
#define EB_DL_URL_MAX     2048
#define EB_DL_PATH_MAX    512
#define EB_DL_NAME_MAX    256

typedef enum { EB_DL_PENDING, EB_DL_DOWNLOADING, EB_DL_PAUSED, EB_DL_COMPLETE, EB_DL_FAILED, EB_DL_CANCELLED } eb_dl_state_t;

typedef struct {
    int id; char url[EB_DL_URL_MAX]; char filename[EB_DL_NAME_MAX]; char save_path[EB_DL_PATH_MAX];
    char mime_type[128]; eb_dl_state_t state; size_t total_bytes, downloaded_bytes;
    double speed_bps; int progress_percent; uint64_t started_at, completed_at;
    int retry_count; bool resumable; char error[256]; char sha256[65];
} eb_download_t;

typedef struct {
    eb_download_t downloads[EB_DL_MAX]; int count, next_id;
    char default_dir[EB_DL_PATH_MAX]; int max_concurrent, active_count;
    bool auto_open, virus_scan; size_t total_downloaded;
} eb_download_mgr_t;

void  eb_dl_init(eb_download_mgr_t *dm, const char *dir);
void  eb_dl_destroy(eb_download_mgr_t *dm);
int   eb_dl_start(eb_download_mgr_t *dm, const char *url, const char *filename);
bool  eb_dl_pause(eb_download_mgr_t *dm, int id);
bool  eb_dl_resume(eb_download_mgr_t *dm, int id);
bool  eb_dl_cancel(eb_download_mgr_t *dm, int id);
bool  eb_dl_retry(eb_download_mgr_t *dm, int id);
bool  eb_dl_remove(eb_download_mgr_t *dm, int id);
eb_download_t *eb_dl_get(eb_download_mgr_t *dm, int id);
void  eb_dl_tick(eb_download_mgr_t *dm);
#endif
