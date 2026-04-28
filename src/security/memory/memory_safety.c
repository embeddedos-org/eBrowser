// SPDX-License-Identifier: MIT
#include "eBrowser/memory_safety.h"
#include "eBrowser/compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static uint64_t mem_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

void eb_mem_safety_init(eb_mem_safety_t *ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->canaries = true;
    ctx->quarantine_on = true;
    ctx->zero_on_free = true;
    ctx->limit = (size_t)-1;
}

void eb_mem_safety_set_limit(eb_mem_safety_t *ctx, size_t limit) {
    if (ctx) ctx->limit = limit;
}

static eb_alloc_info_t *find_alloc(eb_mem_safety_t *ctx, const void *ptr) {
    for (int i = 0; i < ctx->alloc_count; i++)
        if (ctx->allocs[i].ptr == ptr) return &ctx->allocs[i];
    return NULL;
}

void *eb_mem_alloc(eb_mem_safety_t *ctx, size_t size,
                   const char *file, int line, const char *func) {
    if (!ctx || size == 0) return NULL;
    if (ctx->total_alloc + size > ctx->limit) return NULL;
    if (ctx->alloc_count >= EB_MEM_MAX_ALLOCS) return NULL;

    size_t total = sizeof(uint32_t) + size + sizeof(uint32_t);
    uint8_t *raw = (uint8_t *)malloc(total);
    if (!raw) return NULL;
    memset(raw, EB_MEM_UNINIT_FILL, total);

    *(uint32_t *)raw = EB_MEM_CANARY;
    *(uint32_t *)(raw + sizeof(uint32_t) + size) = EB_MEM_CANARY;
    void *user = raw + sizeof(uint32_t);

    eb_alloc_info_t *info = &ctx->allocs[ctx->alloc_count++];
    info->ptr = user; info->size = total; info->requested = size;
    info->canary_head = info->canary_tail = EB_MEM_CANARY;
    info->file = file; info->line = line; info->func = func;
    info->timestamp = mem_now(); info->freed = false; info->id = ctx->next_id++;
    ctx->total_alloc += size;
    if (ctx->total_alloc > ctx->peak_alloc) ctx->peak_alloc = ctx->total_alloc;
    return user;
}

void *eb_mem_calloc(eb_mem_safety_t *ctx, size_t count, size_t size,
                    const char *file, int line, const char *func) {
    size_t total = count * size;
    if (count && total / count != size) return NULL;
    void *p = eb_mem_alloc(ctx, total, file, line, func);
    if (p) memset(p, 0, total);
    return p;
}

void *eb_mem_realloc(eb_mem_safety_t *ctx, void *ptr, size_t nsz,
                     const char *file, int line, const char *func) {
    if (!ptr) return eb_mem_alloc(ctx, nsz, file, line, func);
    if (!nsz) { eb_mem_free(ctx, ptr, file, line, func); return NULL; }
    eb_alloc_info_t *info = find_alloc(ctx, ptr);
    if (!info || info->freed) return NULL;
    void *np = eb_mem_alloc(ctx, nsz, file, line, func);
    if (!np) return NULL;
    memcpy(np, ptr, info->requested < nsz ? info->requested : nsz);
    eb_mem_free(ctx, ptr, file, line, func);
    return np;
}

void eb_mem_free(eb_mem_safety_t *ctx, void *ptr,
                 const char *file, int line, const char *func) {
    if (!ctx || !ptr) return;
    eb_alloc_info_t *info = find_alloc(ctx, ptr);
    if (!info) { fprintf(stderr, "[MEM] free untracked %p at %s:%d\n", ptr, file, line); return; }
    if (info->freed) {
        fprintf(stderr, "[MEM] DOUBLE-FREE at %s:%d\n", file, line);
        ctx->double_frees++; return;
    }
    uint8_t *raw = (uint8_t *)ptr - sizeof(uint32_t);
    if (*(uint32_t *)raw != EB_MEM_CANARY) { ctx->overflows++; fprintf(stderr, "[MEM] UNDERFLOW at %s:%d\n", file, line); }
    if (*(uint32_t *)((uint8_t *)ptr + info->requested) != EB_MEM_CANARY) { ctx->overflows++; fprintf(stderr, "[MEM] OVERFLOW at %s:%d\n", file, line); }
    if (ctx->zero_on_free) memset(ptr, 0, info->requested);
    memset(ptr, EB_MEM_FREED_FILL, info->requested);
    info->freed = true; info->file = file; info->line = line; info->func = func;
    info->timestamp = mem_now();
    ctx->total_freed += info->requested; ctx->total_alloc -= info->requested;

    if (ctx->quarantine_on && ctx->quar_count < EB_MEM_MAX_QUARANTINE) {
        eb_quarantine_entry_t *q = &ctx->quarantine[ctx->quar_head];
        if (q->ptr) free((uint8_t *)q->ptr - sizeof(uint32_t));
        q->ptr = ptr; q->size = info->size; q->freed_at = mem_now();
        ctx->quar_head = (ctx->quar_head + 1) % EB_MEM_MAX_QUARANTINE;
        if (ctx->quar_count < EB_MEM_MAX_QUARANTINE) ctx->quar_count++;
    } else {
        free(raw);
    }
}

char *eb_mem_strdup(eb_mem_safety_t *ctx, const char *str,
                    const char *file, int line, const char *func) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char *d = (char *)eb_mem_alloc(ctx, len, file, line, func);
    if (d) memcpy(d, str, len);
    return d;
}

bool eb_mem_pool_create(eb_mem_safety_t *ctx, size_t bsz, size_t count) {
    if (!ctx || ctx->pool_count >= EB_MEM_MAX_POOLS) return false;
    eb_mem_pool_t *p = &ctx->pools[ctx->pool_count];
    p->block_size = bsz; p->block_count = count; p->used = 0;
    p->buffer = (uint8_t *)calloc(count, bsz);
    p->used_map = (bool *)calloc(count, sizeof(bool));
    if (!p->buffer || !p->used_map) { free(p->buffer); free(p->used_map); return false; }
    ctx->pool_count++;
    return true;
}

void *eb_mem_pool_alloc(eb_mem_pool_t *p) {
    if (!p) return NULL;
    for (size_t i = 0; i < p->block_count; i++)
        if (!p->used_map[i]) { p->used_map[i] = true; p->used++; return p->buffer + i * p->block_size; }
    return NULL;
}

void eb_mem_pool_free(eb_mem_pool_t *p, void *ptr) {
    if (!p || !ptr) return;
    size_t idx = (size_t)((uint8_t *)ptr - p->buffer) / p->block_size;
    if (idx < p->block_count && p->used_map[idx]) {
        memset(ptr, 0, p->block_size); p->used_map[idx] = false; p->used--;
    }
}

void eb_mem_pool_destroy(eb_mem_pool_t *p) {
    if (!p) return;
    free(p->buffer); free(p->used_map); memset(p, 0, sizeof(*p));
}

bool eb_mem_check_canaries(eb_mem_safety_t *ctx) {
    if (!ctx) return false;
    bool ok = true;
    for (int i = 0; i < ctx->alloc_count; i++) {
        eb_alloc_info_t *a = &ctx->allocs[i];
        if (a->freed) continue;
        uint8_t *raw = (uint8_t *)a->ptr - sizeof(uint32_t);
        if (*(uint32_t *)raw != EB_MEM_CANARY) { ok = false; ctx->overflows++; }
        if (*(uint32_t *)((uint8_t *)a->ptr + a->requested) != EB_MEM_CANARY) { ok = false; ctx->overflows++; }
    }
    return ok;
}

int eb_mem_detect_leaks(eb_mem_safety_t *ctx) {
    if (!ctx) return 0;
    int n = 0;
    for (int i = 0; i < ctx->alloc_count; i++)
        if (!ctx->allocs[i].freed) { n++; fprintf(stderr, "[MEM] LEAK: %zu bytes at %s:%d\n", ctx->allocs[i].requested, ctx->allocs[i].file, ctx->allocs[i].line); }
    return n;
}

void eb_mem_get_stats(const eb_mem_safety_t *ctx, eb_mem_stats_t *s) {
    if (!ctx || !s) return;
    s->current = ctx->total_alloc; s->peak = ctx->peak_alloc;
    s->overflows = ctx->overflows; s->uaf = ctx->uaf_detected;
    s->double_frees = ctx->double_frees; s->leaks = 0;
    for (int i = 0; i < ctx->alloc_count; i++) if (!ctx->allocs[i].freed) s->leaks++;
}

void eb_mem_secure_zero(void *ptr, size_t len) {
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (len--) *p++ = 0;
}

bool eb_mem_is_valid(const eb_mem_safety_t *ctx, const void *ptr) {
    if (!ctx || !ptr) return false;
    for (int i = 0; i < ctx->alloc_count; i++)
        if (ctx->allocs[i].ptr == ptr && !ctx->allocs[i].freed) return true;
    return false;
}

void eb_mem_safety_destroy(eb_mem_safety_t *ctx) {
    if (!ctx) return;
    for (int i = 0; i < ctx->quar_count; i++)
        if (ctx->quarantine[i].ptr) free((uint8_t *)ctx->quarantine[i].ptr - sizeof(uint32_t));
    for (int i = 0; i < ctx->pool_count; i++) eb_mem_pool_destroy(&ctx->pools[i]);
    eb_mem_detect_leaks(ctx);
    memset(ctx, 0, sizeof(*ctx));
}
