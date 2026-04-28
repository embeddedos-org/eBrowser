// SPDX-License-Identifier: MIT
#ifndef eBrowser_MEMORY_SAFETY_H
#define eBrowser_MEMORY_SAFETY_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define EB_MEM_CANARY          0xDEADC0DEu
#define EB_MEM_FREED_FILL      0xFE
#define EB_MEM_UNINIT_FILL     0xCD
#define EB_MEM_MAX_QUARANTINE  256
#define EB_MEM_MAX_ALLOCS      4096
#define EB_MEM_MAX_POOLS       16

typedef struct {
    void *ptr; size_t size, requested; uint32_t canary_head, canary_tail;
    const char *file; int line; const char *func;
    uint64_t timestamp; bool freed; uint32_t id;
} eb_alloc_info_t;

typedef struct { void *ptr; size_t size; uint64_t freed_at; } eb_quarantine_entry_t;

typedef struct {
    uint8_t *buffer; size_t block_size, block_count, used; bool *used_map;
} eb_mem_pool_t;

typedef struct {
    eb_alloc_info_t allocs[EB_MEM_MAX_ALLOCS]; int alloc_count; uint32_t next_id;
    eb_quarantine_entry_t quarantine[EB_MEM_MAX_QUARANTINE]; int quar_count, quar_head;
    size_t total_alloc, peak_alloc, total_freed, limit;
    int overflows, uaf_detected, double_frees;
    bool guard_pages, canaries, quarantine_on, zero_on_free;
    eb_mem_pool_t pools[EB_MEM_MAX_POOLS]; int pool_count;
} eb_mem_safety_t;

#define eb_safe_malloc(ctx, sz) eb_mem_alloc(ctx, sz, __FILE__, __LINE__, __func__)
#define eb_safe_free(ctx, p)    eb_mem_free(ctx, p, __FILE__, __LINE__, __func__)

void  eb_mem_safety_init(eb_mem_safety_t *ctx);
void  eb_mem_safety_destroy(eb_mem_safety_t *ctx);
void  eb_mem_safety_set_limit(eb_mem_safety_t *ctx, size_t limit);
void *eb_mem_alloc(eb_mem_safety_t *ctx, size_t size, const char *file, int line, const char *func);
void *eb_mem_calloc(eb_mem_safety_t *ctx, size_t count, size_t size, const char *file, int line, const char *func);
void *eb_mem_realloc(eb_mem_safety_t *ctx, void *ptr, size_t new_size, const char *file, int line, const char *func);
void  eb_mem_free(eb_mem_safety_t *ctx, void *ptr, const char *file, int line, const char *func);
char *eb_mem_strdup(eb_mem_safety_t *ctx, const char *str, const char *file, int line, const char *func);
bool  eb_mem_pool_create(eb_mem_safety_t *ctx, size_t block_size, size_t count);
void *eb_mem_pool_alloc(eb_mem_pool_t *pool);
void  eb_mem_pool_free(eb_mem_pool_t *pool, void *ptr);
void  eb_mem_pool_destroy(eb_mem_pool_t *pool);
bool  eb_mem_check_canaries(eb_mem_safety_t *ctx);
int   eb_mem_detect_leaks(eb_mem_safety_t *ctx);
typedef struct { size_t current, peak; int overflows, uaf, double_frees, leaks; } eb_mem_stats_t;
void  eb_mem_get_stats(const eb_mem_safety_t *ctx, eb_mem_stats_t *stats);
void  eb_mem_secure_zero(void *ptr, size_t len);
bool  eb_mem_is_valid(const eb_mem_safety_t *ctx, const void *ptr);
#endif
