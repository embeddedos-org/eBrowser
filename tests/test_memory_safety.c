// SPDX-License-Identifier: MIT
// Memory safety module tests
#include "eBrowser/memory_safety.h"
#include <stdio.h>
#include <string.h>

static int s_pass = 0, s_fail = 0;
#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %s... ", #name); name(); printf("PASS\n"); s_pass++; } while(0)
#define ASSERT(cond) do { if(!(cond)) { printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); s_fail++; return; } } while(0)

// --- Init/Destroy ---
TEST(test_init) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    ASSERT(ctx.canaries == true);
    ASSERT(ctx.quarantine_on == true);
    ASSERT(ctx.zero_on_free == true);
    ASSERT(ctx.alloc_count == 0);
    ASSERT(ctx.total_alloc == 0);
    ASSERT(ctx.peak_alloc == 0);
    ASSERT(ctx.overflows == 0);
    ASSERT(ctx.double_frees == 0);
    ASSERT(ctx.uaf_detected == 0);
    eb_mem_safety_destroy(&ctx);
}

TEST(test_init_null) {
    eb_mem_safety_init(NULL); /* Should not crash */
    eb_mem_safety_destroy(NULL);
}

// --- Basic allocation ---
TEST(test_alloc_free) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    void *p = eb_safe_malloc(&ctx, 64);
    ASSERT(p != NULL);
    ASSERT(ctx.alloc_count == 1);
    ASSERT(ctx.total_alloc == 64);
    eb_safe_free(&ctx, p);
    ASSERT(ctx.total_alloc == 0);
    ASSERT(ctx.total_freed == 64);
    eb_mem_safety_destroy(&ctx);
}

TEST(test_alloc_zero_size) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    void *p = eb_safe_malloc(&ctx, 0);
    ASSERT(p == NULL);
    ASSERT(ctx.alloc_count == 0);
    eb_mem_safety_destroy(&ctx);
}

TEST(test_alloc_null_ctx) {
    void *p = eb_mem_alloc(NULL, 64, __FILE__, __LINE__, __func__);
    ASSERT(p == NULL);
}

TEST(test_multiple_allocs) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    void *a = eb_safe_malloc(&ctx, 32);
    void *b = eb_safe_malloc(&ctx, 64);
    void *c = eb_safe_malloc(&ctx, 128);
    ASSERT(a != NULL && b != NULL && c != NULL);
    ASSERT(ctx.alloc_count == 3);
    ASSERT(ctx.total_alloc == 32 + 64 + 128);
    ASSERT(a != b && b != c && a != c); /* Distinct pointers */
    eb_safe_free(&ctx, b);
    ASSERT(ctx.total_alloc == 32 + 128);
    eb_safe_free(&ctx, a);
    eb_safe_free(&ctx, c);
    ASSERT(ctx.total_alloc == 0);
    eb_mem_safety_destroy(&ctx);
}

// --- Canary detection ---
TEST(test_canary_intact) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    void *p = eb_safe_malloc(&ctx, 64);
    ASSERT(p != NULL);
    /* Write within bounds - canaries should be fine */
    memset(p, 'A', 64);
    ASSERT(eb_mem_check_canaries(&ctx) == true);
    eb_safe_free(&ctx, p);
    eb_mem_safety_destroy(&ctx);
}

TEST(test_canary_overflow_detected) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    void *p = eb_safe_malloc(&ctx, 32);
    ASSERT(p != NULL);
    int prev_overflows = ctx.overflows;
    /* Deliberately write past the allocation to corrupt tail canary */
    memset(p, 'X', 32 + sizeof(uint32_t)); /* Overwrite tail canary */
    ASSERT(eb_mem_check_canaries(&ctx) == false);
    ASSERT(ctx.overflows > prev_overflows);
    /* Free will also detect the corruption */
    eb_safe_free(&ctx, p);
    eb_mem_safety_destroy(&ctx);
}

// --- Double-free detection ---
TEST(test_double_free_detected) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    ctx.quarantine_on = false; /* Disable quarantine so first free actually frees */
    void *p = eb_safe_malloc(&ctx, 64);
    ASSERT(p != NULL);
    eb_safe_free(&ctx, p);
    int prev = ctx.double_frees;
    /* Second free should be detected */
    eb_safe_free(&ctx, p);
    ASSERT(ctx.double_frees == prev + 1);
    eb_mem_safety_destroy(&ctx);
}

// --- Memory limit ---
TEST(test_memory_limit) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    eb_mem_safety_set_limit(&ctx, 100);
    void *a = eb_safe_malloc(&ctx, 60);
    ASSERT(a != NULL);
    void *b = eb_safe_malloc(&ctx, 60); /* Would exceed 100 byte limit */
    ASSERT(b == NULL);
    ASSERT(ctx.total_alloc == 60);
    eb_safe_free(&ctx, a);
    eb_mem_safety_destroy(&ctx);
}

// --- Peak tracking ---
TEST(test_peak_tracking) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    void *a = eb_safe_malloc(&ctx, 100);
    void *b = eb_safe_malloc(&ctx, 200);
    ASSERT(ctx.peak_alloc == 300);
    eb_safe_free(&ctx, b);
    ASSERT(ctx.peak_alloc == 300); /* Peak stays */
    ASSERT(ctx.total_alloc == 100);
    eb_safe_free(&ctx, a);
    eb_mem_safety_destroy(&ctx);
}

// --- Leak detection ---
TEST(test_leak_detection) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    void *a = eb_safe_malloc(&ctx, 32);
    void *b = eb_safe_malloc(&ctx, 64);
    eb_safe_free(&ctx, a);
    /* b is leaked */
    int leaks = eb_mem_detect_leaks(&ctx);
    ASSERT(leaks == 1);
    eb_safe_free(&ctx, b);
    leaks = eb_mem_detect_leaks(&ctx);
    ASSERT(leaks == 0);
    eb_mem_safety_destroy(&ctx);
}

// --- Pointer validity ---
TEST(test_pointer_validity) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    void *p = eb_safe_malloc(&ctx, 64);
    ASSERT(eb_mem_is_valid(&ctx, p) == true);
    int dummy;
    ASSERT(eb_mem_is_valid(&ctx, &dummy) == false);
    ASSERT(eb_mem_is_valid(&ctx, NULL) == false);
    eb_safe_free(&ctx, p);
    ASSERT(eb_mem_is_valid(&ctx, p) == false); /* Freed = invalid */
    eb_mem_safety_destroy(&ctx);
}

// --- Stats ---
TEST(test_stats) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    void *a = eb_safe_malloc(&ctx, 100);
    void *b = eb_safe_malloc(&ctx, 200);
    eb_safe_free(&ctx, a);
    eb_mem_stats_t stats;
    eb_mem_get_stats(&ctx, &stats);
    ASSERT(stats.current == 200);
    ASSERT(stats.peak == 300);
    ASSERT(stats.leaks == 1); /* b is still allocated */
    ASSERT(stats.overflows == 0);
    ASSERT(stats.double_frees == 0);
    eb_safe_free(&ctx, b);
    eb_mem_safety_destroy(&ctx);
}

// --- Calloc ---
TEST(test_calloc_zeroed) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    uint8_t *p = (uint8_t *)eb_mem_calloc(&ctx, 10, 10, __FILE__, __LINE__, __func__);
    ASSERT(p != NULL);
    /* Should be zero-filled */
    for (int i = 0; i < 100; i++) ASSERT(p[i] == 0);
    eb_safe_free(&ctx, p);
    eb_mem_safety_destroy(&ctx);
}

// --- Realloc ---
TEST(test_realloc_grow) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    char *p = (char *)eb_safe_malloc(&ctx, 16);
    ASSERT(p != NULL);
    memcpy(p, "Hello, World!\0\0\0", 16);
    char *p2 = (char *)eb_mem_realloc(&ctx, p, 64, __FILE__, __LINE__, __func__);
    ASSERT(p2 != NULL);
    ASSERT(memcmp(p2, "Hello, World!", 13) == 0); /* Data preserved */
    eb_safe_free(&ctx, p2);
    eb_mem_safety_destroy(&ctx);
}

TEST(test_realloc_null_allocs) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    char *p = (char *)eb_mem_realloc(&ctx, NULL, 32, __FILE__, __LINE__, __func__);
    ASSERT(p != NULL); /* realloc(NULL, n) == malloc(n) */
    eb_safe_free(&ctx, p);
    eb_mem_safety_destroy(&ctx);
}

// --- Strdup ---
TEST(test_strdup) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    char *s = eb_mem_strdup(&ctx, "eBrowser", __FILE__, __LINE__, __func__);
    ASSERT(s != NULL);
    ASSERT(strcmp(s, "eBrowser") == 0);
    ASSERT(strlen(s) == 8);
    eb_safe_free(&ctx, s);
    eb_mem_safety_destroy(&ctx);
}

TEST(test_strdup_null) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    char *s = eb_mem_strdup(&ctx, NULL, __FILE__, __LINE__, __func__);
    ASSERT(s == NULL);
    eb_mem_safety_destroy(&ctx);
}

// --- Memory Pool ---
TEST(test_pool_basic) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    ASSERT(eb_mem_pool_create(&ctx, 64, 8) == true);
    ASSERT(ctx.pool_count == 1);
    eb_mem_pool_t *pool = &ctx.pools[0];
    void *a = eb_mem_pool_alloc(pool);
    void *b = eb_mem_pool_alloc(pool);
    ASSERT(a != NULL && b != NULL);
    ASSERT(a != b);
    ASSERT(pool->used == 2);
    eb_mem_pool_free(pool, a);
    ASSERT(pool->used == 1);
    /* Re-allocate should reuse freed slot */
    void *c = eb_mem_pool_alloc(pool);
    ASSERT(c == a); /* Should get same block back */
    ASSERT(pool->used == 2);
    eb_mem_pool_free(pool, b);
    eb_mem_pool_free(pool, c);
    eb_mem_safety_destroy(&ctx);
}

TEST(test_pool_exhaustion) {
    eb_mem_safety_t ctx;
    eb_mem_safety_init(&ctx);
    ASSERT(eb_mem_pool_create(&ctx, 32, 4) == true);
    eb_mem_pool_t *pool = &ctx.pools[0];
    void *ptrs[4];
    for (int i = 0; i < 4; i++) {
        ptrs[i] = eb_mem_pool_alloc(pool);
        ASSERT(ptrs[i] != NULL);
    }
    ASSERT(pool->used == 4);
    void *overflow = eb_mem_pool_alloc(pool);
    ASSERT(overflow == NULL); /* Pool full */
    for (int i = 0; i < 4; i++) eb_mem_pool_free(pool, ptrs[i]);
    eb_mem_safety_destroy(&ctx);
}

// --- Secure zero ---
TEST(test_secure_zero) {
    uint8_t buf[32];
    memset(buf, 0xFF, sizeof(buf));
    eb_mem_secure_zero(buf, sizeof(buf));
    for (int i = 0; i < 32; i++) ASSERT(buf[i] == 0);
}

int main(void) {
    printf("=== Memory Safety Tests ===\n");
    RUN(test_init); RUN(test_init_null);
    RUN(test_alloc_free); RUN(test_alloc_zero_size);
    RUN(test_alloc_null_ctx); RUN(test_multiple_allocs);
    RUN(test_canary_intact); RUN(test_canary_overflow_detected);
    RUN(test_double_free_detected);
    RUN(test_memory_limit); RUN(test_peak_tracking);
    RUN(test_leak_detection); RUN(test_pointer_validity);
    RUN(test_stats);
    RUN(test_calloc_zeroed); RUN(test_realloc_grow);
    RUN(test_realloc_null_allocs);
    RUN(test_strdup); RUN(test_strdup_null);
    RUN(test_pool_basic); RUN(test_pool_exhaustion);
    RUN(test_secure_zero);
    printf("\nResults: %d passed, %d failed\n", s_pass, s_fail);
    return s_fail > 0 ? 1 : 0;
}
