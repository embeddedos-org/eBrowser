// SPDX-License-Identifier: MIT
/* Unit tests for the LRU cache in src/perf/perf.c.
 *
 * eb_perf had no ctest target at all, so none of this code was covered: the
 * three executables that link it (benchmark, http2_server, load_test_combined)
 * are build targets, not registered tests. The first two cases below are
 * regressions — both crashed or hung before the guards in eb_cache_put() and
 * eb_cache_get() were repaired. */
#include "eBrowser/perf.h"
#include <stdio.h>
#include <string.h>

static int s_pass = 0, s_fail = 0;
#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %s... ", #name); name(); printf("PASS\n"); s_pass++; } while(0)
#define ASSERT(cond) do { if(!(cond)) { printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); s_fail++; return; } } while(0)

/* Regression: the guard that rejects a NULL cache dereferenced it first. */
TEST(test_get_null_cache_does_not_crash) {
    ASSERT(eb_cache_get(NULL, "k", NULL) == NULL);
}

TEST(test_get_null_key_counts_a_miss) {
    eb_lru_cache_t c;
    eb_cache_init(&c, 8, 4096);
    ASSERT(eb_cache_get(&c, NULL, NULL) == NULL);
    ASSERT(c.misses == 1);
    eb_cache_destroy(&c);
}

/* Regression: a value that cannot ever fit the byte budget made the eviction
 * loop spin against an already-empty cache. */
TEST(test_put_larger_than_byte_budget_is_refused) {
    eb_lru_cache_t c;
    eb_cache_init(&c, 8, 64);
    char big[256];
    memset(big, 'x', sizeof(big));
    ASSERT(eb_cache_put(&c, "big", big, sizeof(big), 0) == false);
    ASSERT(c.count == 0);
    eb_cache_destroy(&c);
}

/* Regression: max_entries == 0 makes `count >= max_entries` true forever. */
TEST(test_put_into_zero_entry_cache_is_refused) {
    eb_lru_cache_t c;
    eb_cache_init(&c, 0, 0);
    ASSERT(eb_cache_put(&c, "k", "v", 1, 0) == false);
    ASSERT(c.count == 0);
    eb_cache_destroy(&c);
}

TEST(test_put_get_remove_roundtrip) {
    eb_lru_cache_t c;
    eb_cache_init(&c, 4, 4096);
    ASSERT(eb_cache_put(&c, "a", "alpha", 6, 0) == true);
    size_t sz = 0;
    void *v = eb_cache_get(&c, "a", &sz);
    ASSERT(v != NULL);
    ASSERT(sz == 6);
    ASSERT(strcmp((const char *)v, "alpha") == 0);
    ASSERT(c.hits == 1);
    ASSERT(eb_cache_get(&c, "missing", NULL) == NULL);
    ASSERT(c.misses == 1);
    ASSERT(eb_cache_remove(&c, "a") == true);
    ASSERT(eb_cache_get(&c, "a", NULL) == NULL);
    eb_cache_destroy(&c);
}

/* A put that fits the budget still evicts to make room for itself. */
TEST(test_put_evicts_to_make_room) {
    eb_lru_cache_t c;
    eb_cache_init(&c, 8, 16);
    ASSERT(eb_cache_put(&c, "a", "0123456789", 10, 0) == true);
    ASSERT(c.count == 1);
    ASSERT(eb_cache_put(&c, "b", "0123456789", 10, 0) == true);
    ASSERT(c.count == 1);
    ASSERT(eb_cache_get(&c, "b", NULL) != NULL);
    eb_cache_destroy(&c);
}

int main(void) {
    printf("=== Performance LRU Cache Tests ===\n");
    RUN(test_get_null_cache_does_not_crash);
    RUN(test_get_null_key_counts_a_miss);
    RUN(test_put_larger_than_byte_budget_is_refused);
    RUN(test_put_into_zero_entry_cache_is_refused);
    RUN(test_put_get_remove_roundtrip);
    RUN(test_put_evicts_to_make_room);
    printf("\nResults: %d passed, %d failed\n", s_pass, s_fail);
    return s_fail > 0 ? 1 : 0;
}
