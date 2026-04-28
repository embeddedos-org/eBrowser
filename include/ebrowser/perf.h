// SPDX-License-Identifier: MIT
#ifndef eBrowser_PERF_H
#define eBrowser_PERF_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define EB_THREAD_MAX_WORKERS 16
#define EB_THREAD_MAX_QUEUE   256
#define EB_CACHE_MAX_ENTRIES  512
#define EB_PREFETCH_MAX_QUEUE 64
#define EB_H2_MAX_STREAMS     128

typedef void (*eb_task_fn)(void *arg);
typedef struct { eb_task_fn func; void *arg; int priority; } eb_thread_task_t;

typedef struct {
    int worker_count, queue_head, queue_tail, queue_count, active_tasks;
    bool shutdown; uint64_t tasks_completed;
    eb_thread_task_t queue[EB_THREAD_MAX_QUEUE]; void *internal;
} eb_thread_pool_t;

bool  eb_threadpool_create(eb_thread_pool_t *pool, int workers);
bool  eb_threadpool_submit(eb_thread_pool_t *pool, eb_task_fn func, void *arg, int priority);
void  eb_threadpool_wait_all(eb_thread_pool_t *pool);
void  eb_threadpool_destroy(eb_thread_pool_t *pool);

typedef struct eb_lru_node {
    char key[256]; void *value; size_t value_size; uint64_t last_access, created;
    uint32_t ttl_sec; struct eb_lru_node *prev, *next;
} eb_lru_node_t;

typedef struct {
    eb_lru_node_t entries[EB_CACHE_MAX_ENTRIES]; int count, max_entries;
    size_t max_bytes, current_bytes; uint64_t hits, misses;
    eb_lru_node_t *head, *tail;
} eb_lru_cache_t;

void  eb_cache_init(eb_lru_cache_t *c, int max_entries, size_t max_bytes);
bool  eb_cache_put(eb_lru_cache_t *c, const char *key, const void *val, size_t sz, uint32_t ttl);
void *eb_cache_get(eb_lru_cache_t *c, const char *key, size_t *sz);
bool  eb_cache_remove(eb_lru_cache_t *c, const char *key);
void  eb_cache_evict_lru(eb_lru_cache_t *c);
void  eb_cache_flush(eb_lru_cache_t *c);
void  eb_cache_destroy(eb_lru_cache_t *c);

typedef enum { EB_COMPRESS_NONE, EB_COMPRESS_GZIP, EB_COMPRESS_BROTLI, EB_COMPRESS_ZSTD, EB_COMPRESS_DEFLATE } eb_compress_t;
int   eb_decompress(eb_compress_t type, const uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len);
int   eb_compress_data(eb_compress_t type, const uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len, int level);
eb_compress_t eb_detect_encoding(const char *content_encoding);

typedef struct { char url[1024]; int priority; bool completed; uint8_t *data; size_t data_len; } eb_prefetch_item_t;
typedef struct {
    eb_prefetch_item_t queue[EB_PREFETCH_MAX_QUEUE]; int count;
    bool enabled; int max_concurrent, active; uint64_t total_prefetched; size_t bytes_saved;
} eb_prefetcher_t;

void  eb_prefetch_init(eb_prefetcher_t *pf);
bool  eb_prefetch_add(eb_prefetcher_t *pf, const char *url, int priority);
void  eb_prefetch_tick(eb_prefetcher_t *pf);
void *eb_prefetch_get(eb_prefetcher_t *pf, const char *url, size_t *len);
void  eb_prefetch_destroy(eb_prefetcher_t *pf);

typedef struct {
    uint32_t stream_id; char url[1024]; uint8_t *data; size_t data_len; int status; bool complete;
} eb_h2_stream_t;

typedef struct {
    eb_h2_stream_t streams[EB_H2_MAX_STREAMS]; int stream_count; uint32_t next_stream_id;
    bool connected; int max_concurrent, window_size; void *internal;
} eb_h2_connection_t;

bool  eb_h2_connect(eb_h2_connection_t *conn, const char *host, uint16_t port);
int   eb_h2_request(eb_h2_connection_t *conn, const char *method, const char *path, const char *headers);
int   eb_h2_read_response(eb_h2_connection_t *conn, uint32_t stream_id, uint8_t *buf, size_t buf_len);
void  eb_h2_close(eb_h2_connection_t *conn);

typedef struct {
    uint64_t dns_us, connect_us, tls_us, first_byte_us, download_us;
    uint64_t parse_us, layout_us, render_us, js_us, total_us;
    size_t bytes_transferred, bytes_decoded; int requests, cache_hits;
} eb_perf_timing_t;

void  eb_perf_start(eb_perf_timing_t *t);
void  eb_perf_mark(eb_perf_timing_t *t, const char *phase);
void  eb_perf_end(eb_perf_timing_t *t);
void  eb_perf_print(const eb_perf_timing_t *t);
#endif
