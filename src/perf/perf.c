// SPDX-License-Identifier: MIT
#include "eBrowser/perf.h"
#include "eBrowser/compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#ifdef __linux__
#include <pthread.h>
#include <unistd.h>
#endif

static uint64_t perf_now_us(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

/* === Thread Pool === */
#ifdef __linux__
typedef struct {
    pthread_t *threads;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    eb_thread_pool_t *pool;
} eb_tp_internal_t;

static void *worker_func(void *arg) {
    eb_tp_internal_t *ti = (eb_tp_internal_t *)arg;
    eb_thread_pool_t *p = ti->pool;
    while (1) {
        pthread_mutex_lock(&ti->mutex);
        while (p->queue_count == 0 && !p->shutdown)
            pthread_cond_wait(&ti->cond, &ti->mutex);
        if (p->shutdown && p->queue_count == 0) {
            pthread_mutex_unlock(&ti->mutex);
            break;
        }
        eb_thread_task_t task = p->queue[p->queue_head];
        p->queue_head = (p->queue_head + 1) % EB_THREAD_MAX_QUEUE;
        p->queue_count--;
        p->active_tasks++;
        pthread_mutex_unlock(&ti->mutex);

        if (task.func) task.func(task.arg);

        pthread_mutex_lock(&ti->mutex);
        p->active_tasks--;
        p->tasks_completed++;
        pthread_cond_broadcast(&ti->cond);
        pthread_mutex_unlock(&ti->mutex);
    }
    return NULL;
}
#endif

bool eb_threadpool_create(eb_thread_pool_t *p, int workers) {
    if (!p || workers <= 0 || workers > EB_THREAD_MAX_WORKERS) return false;
    memset(p, 0, sizeof(*p));
    p->worker_count = workers;
#ifdef __linux__
    eb_tp_internal_t *ti = (eb_tp_internal_t *)calloc(1, sizeof(*ti));
    if (!ti) return false;
    ti->pool = p;
    pthread_mutex_init(&ti->mutex, NULL);
    pthread_cond_init(&ti->cond, NULL);
    ti->threads = (pthread_t *)calloc(workers, sizeof(pthread_t));
    if (!ti->threads) { free(ti); return false; }
    p->internal = ti;
    for (int i = 0; i < workers; i++)
        pthread_create(&ti->threads[i], NULL, worker_func, ti);
#endif
    return true;
}

bool eb_threadpool_submit(eb_thread_pool_t *p, eb_task_fn func, void *arg, int prio) {
    if (!p || !func || p->queue_count >= EB_THREAD_MAX_QUEUE) return false;
#ifdef __linux__
    eb_tp_internal_t *ti = (eb_tp_internal_t *)p->internal;
    pthread_mutex_lock(&ti->mutex);
    p->queue[p->queue_tail].func = func;
    p->queue[p->queue_tail].arg = arg;
    p->queue[p->queue_tail].priority = prio;
    p->queue_tail = (p->queue_tail + 1) % EB_THREAD_MAX_QUEUE;
    p->queue_count++;
    pthread_cond_signal(&ti->cond);
    pthread_mutex_unlock(&ti->mutex);
#else
    /* Single-threaded fallback */
    func(arg);
    p->tasks_completed++;
#endif
    return true;
}

void eb_threadpool_wait_all(eb_thread_pool_t *p) {
    if (!p) return;
#ifdef __linux__
    eb_tp_internal_t *ti = (eb_tp_internal_t *)p->internal;
    if (!ti) return;
    pthread_mutex_lock(&ti->mutex);
    while (p->queue_count > 0 || p->active_tasks > 0)
        pthread_cond_wait(&ti->cond, &ti->mutex);
    pthread_mutex_unlock(&ti->mutex);
#endif
}

void eb_threadpool_destroy(eb_thread_pool_t *p) {
    if (!p) return;
#ifdef __linux__
    eb_tp_internal_t *ti = (eb_tp_internal_t *)p->internal;
    if (!ti) return;
    pthread_mutex_lock(&ti->mutex);
    p->shutdown = true;
    pthread_cond_broadcast(&ti->cond);
    pthread_mutex_unlock(&ti->mutex);
    for (int i = 0; i < p->worker_count; i++)
        pthread_join(ti->threads[i], NULL);
    pthread_mutex_destroy(&ti->mutex);
    pthread_cond_destroy(&ti->cond);
    free(ti->threads);
    free(ti);
#endif
    p->internal = NULL;
}

/* === LRU Cache === */
void eb_cache_init(eb_lru_cache_t *c, int max, size_t max_bytes) {
    if (!c) return; memset(c, 0, sizeof(*c));
    c->max_entries = max > EB_CACHE_MAX_ENTRIES ? EB_CACHE_MAX_ENTRIES : max;
    c->max_bytes = max_bytes;
}

static void cache_detach(eb_lru_cache_t *c, eb_lru_node_t *n) {
    if (n->prev) n->prev->next = n->next;
    if (n->next) n->next->prev = n->prev;
    if (c->head == n) c->head = n->next;
    if (c->tail == n) c->tail = n->prev;
    n->prev = n->next = NULL;
}

static void cache_push_front(eb_lru_cache_t *c, eb_lru_node_t *n) {
    n->prev = NULL; n->next = c->head;
    if (c->head) c->head->prev = n;
    c->head = n;
    if (!c->tail) c->tail = n;
}

bool eb_cache_put(eb_lru_cache_t *c, const char *key, const void *val, size_t sz, uint32_t ttl) {
    if (!c || !key || !val) return false;
    /* Check if already exists */
    for (int i = 0; i < c->count; i++) {
        if (strcmp(c->entries[i].key, key) == 0) {
            /* Update existing */
            eb_lru_node_t *n = &c->entries[i];
            c->current_bytes -= n->value_size;
            free(n->value);
            n->value = malloc(sz); if (!n->value) return false;
            memcpy(n->value, val, sz);
            n->value_size = sz; n->ttl_sec = ttl;
            n->last_access = perf_now_us();
            c->current_bytes += sz;
            cache_detach(c, n); cache_push_front(c, n);
            return true;
        }
    }
    /* Evict if needed */
    while (c->count >= c->max_entries || (c->max_bytes && c->current_bytes + sz > c->max_bytes))
        eb_cache_evict_lru(c);
    if (c->count >= EB_CACHE_MAX_ENTRIES) return false;

    eb_lru_node_t *n = &c->entries[c->count];
    memset(n, 0, sizeof(*n));
    strncpy(n->key, key, sizeof(n->key)-1);
    n->value = malloc(sz); if (!n->value) return false;
    memcpy(n->value, val, sz);
    n->value_size = sz; n->ttl_sec = ttl;
    n->created = n->last_access = perf_now_us();
    c->current_bytes += sz;
    cache_push_front(c, n);
    c->count++;
    return true;
}

void *eb_cache_get(eb_lru_cache_t *c, const char *key, size_t *sz) {
    if (!c || !key) { c->misses++; return NULL; }
    uint64_t now = perf_now_us();
    for (int i = 0; i < c->count; i++) {
        eb_lru_node_t *n = &c->entries[i];
        if (strcmp(n->key, key) != 0) continue;
        /* Check TTL */
        if (n->ttl_sec > 0 && (now - n->created) / 1000000 > n->ttl_sec) {
            eb_cache_remove(c, key); c->misses++; return NULL;
        }
        n->last_access = now;
        cache_detach(c, n); cache_push_front(c, n);
        c->hits++;
        if (sz) *sz = n->value_size;
        return n->value;
    }
    c->misses++;
    return NULL;
}

bool eb_cache_remove(eb_lru_cache_t *c, const char *key) {
    if (!c || !key) return false;
    for (int i = 0; i < c->count; i++) {
        if (strcmp(c->entries[i].key, key) == 0) {
            cache_detach(c, &c->entries[i]);
            c->current_bytes -= c->entries[i].value_size;
            free(c->entries[i].value);
            for (int j = i; j < c->count-1; j++) c->entries[j] = c->entries[j+1];
            c->count--; return true;
        }
    }
    return false;
}

void eb_cache_evict_lru(eb_lru_cache_t *c) {
    if (!c || !c->tail) return;
    eb_cache_remove(c, c->tail->key);
}

void eb_cache_flush(eb_lru_cache_t *c) {
    if (!c) return;
    for (int i = 0; i < c->count; i++) free(c->entries[i].value);
    memset(c, 0, sizeof(*c));
}

void eb_cache_destroy(eb_lru_cache_t *c) { eb_cache_flush(c); }

/* === Compression === */
eb_compress_t eb_detect_encoding(const char *enc) {
    if (!enc) return EB_COMPRESS_NONE;
    if (strstr(enc, "br")) return EB_COMPRESS_BROTLI;
    if (strstr(enc, "zstd")) return EB_COMPRESS_ZSTD;
    if (strstr(enc, "gzip")) return EB_COMPRESS_GZIP;
    if (strstr(enc, "deflate")) return EB_COMPRESS_DEFLATE;
    return EB_COMPRESS_NONE;
}

int eb_decompress(eb_compress_t type, const uint8_t *in, size_t in_len,
                   uint8_t *out, size_t *out_len) {
    if (!in || !out || !out_len) return -1;
    /* In production: link against zlib/brotli/zstd libraries.
       Fallback: pass through uncompressed data */
    if (type == EB_COMPRESS_NONE) {
        if (*out_len < in_len) return -1;
        memcpy(out, in, in_len); *out_len = in_len; return 0;
    }
    /* Stub for compressed formats - requires linking actual libraries */
    return -1;
}

int eb_compress_data(eb_compress_t type, const uint8_t *in, size_t in_len,
                      uint8_t *out, size_t *out_len, int level) {
    (void)level;
    return eb_decompress(type, in, in_len, out, out_len);
}

/* === Prefetcher === */
void eb_prefetch_init(eb_prefetcher_t *pf) {
    if (!pf) return; memset(pf, 0, sizeof(*pf));
    pf->enabled = true; pf->max_concurrent = 4;
}

bool eb_prefetch_add(eb_prefetcher_t *pf, const char *url, int prio) {
    if (!pf || !pf->enabled || !url || pf->count >= EB_PREFETCH_MAX_QUEUE) return false;
    /* Check for duplicate */
    for (int i = 0; i < pf->count; i++)
        if (strcmp(pf->queue[i].url, url) == 0) return true;
    eb_prefetch_item_t *it = &pf->queue[pf->count];
    strncpy(it->url, url, sizeof(it->url)-1);
    it->priority = prio; it->completed = false;
    pf->count++; return true;
}

void *eb_prefetch_get(eb_prefetcher_t *pf, const char *url, size_t *len) {
    if (!pf || !url) return NULL;
    for (int i = 0; i < pf->count; i++)
        if (pf->queue[i].completed && strcmp(pf->queue[i].url, url) == 0) {
            if (len) *len = pf->queue[i].data_len;
            return pf->queue[i].data;
        }
    return NULL;
}

void eb_prefetch_tick(eb_prefetcher_t *pf) {
    if (!pf || !pf->enabled) return;
    /* In production: initiate HTTP requests for pending items */
}

void eb_prefetch_destroy(eb_prefetcher_t *pf) {
    if (!pf) return;
    for (int i = 0; i < pf->count; i++) free(pf->queue[i].data);
    memset(pf, 0, sizeof(*pf));
}

/* === HTTP/2 === */
bool eb_h2_connect(eb_h2_connection_t *c, const char *host, uint16_t port) {
    if (!c || !host) return false;
    memset(c, 0, sizeof(*c));
    c->next_stream_id = 1; c->max_concurrent = 100;
    c->window_size = 65535; c->connected = true;
    return true;
}

int eb_h2_request(eb_h2_connection_t *c, const char *method, const char *path, const char *hdrs) {
    if (!c || !c->connected || c->stream_count >= EB_H2_MAX_STREAMS) return -1;
    eb_h2_stream_t *s = &c->streams[c->stream_count];
    s->stream_id = c->next_stream_id; c->next_stream_id += 2;
    snprintf(s->url, sizeof(s->url), "%s %s", method, path);
    s->complete = false; c->stream_count++;
    return (int)s->stream_id;
}

int eb_h2_read_response(eb_h2_connection_t *c, uint32_t sid, uint8_t *buf, size_t max) {
    if (!c) return -1;
    for (int i = 0; i < c->stream_count; i++)
        if (c->streams[i].stream_id == sid && c->streams[i].complete) {
            size_t n = c->streams[i].data_len < max ? c->streams[i].data_len : max;
            if (c->streams[i].data) memcpy(buf, c->streams[i].data, n);
            return (int)n;
        }
    return 0;
}

void eb_h2_close(eb_h2_connection_t *c) {
    if (!c) return;
    for (int i = 0; i < c->stream_count; i++) free(c->streams[i].data);
    memset(c, 0, sizeof(*c));
}

/* === Performance Profiling === */
void eb_perf_start(eb_perf_timing_t *t) {
    if (t) { memset(t, 0, sizeof(*t)); t->dns_us = perf_now_us(); }
}
void eb_perf_mark(eb_perf_timing_t *t, const char *phase) { (void)t; (void)phase; }
void eb_perf_end(eb_perf_timing_t *t) {
    if (t) t->total_us = perf_now_us() - t->dns_us;
}
void eb_perf_print(const eb_perf_timing_t *t) {
    if (!t) return;
    printf("[PERF] Total: %llu us | DNS: %llu | Connect: %llu | TLS: %llu\n",
        (unsigned long long)t->total_us, (unsigned long long)t->dns_us,
        (unsigned long long)t->connect_us, (unsigned long long)t->tls_us);
    printf("[PERF] Parse: %llu | Layout: %llu | Render: %llu | JS: %llu\n",
        (unsigned long long)t->parse_us, (unsigned long long)t->layout_us,
        (unsigned long long)t->render_us, (unsigned long long)t->js_us);
}
