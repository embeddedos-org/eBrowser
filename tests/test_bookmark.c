// SPDX-License-Identifier: MIT
#include "eBrowser/bookmark.h"
#include <stdio.h>
#include <string.h>

static int s_pass = 0, s_fail = 0;
#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %s... ", #name); name(); printf("PASS\n"); s_pass++; } while(0)
#define ASSERT(cond) do { if(!(cond)) { printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); s_fail++; return; } } while(0)

TEST(test_init) {
    static eb_bookmark_mgr_t bm;
    eb_bm_init(&bm);
    ASSERT(bm.count == 2); /* Two default folders */
    ASSERT(bm.items[0].type == EB_BM_FOLDER);
    ASSERT(strcmp(bm.items[0].title, "Bookmarks Bar") == 0);
    ASSERT(bm.items[1].type == EB_BM_FOLDER);
    ASSERT(strcmp(bm.items[1].title, "Other Bookmarks") == 0);
    eb_bm_destroy(&bm);
}

TEST(test_add_bookmark) {
    static eb_bookmark_mgr_t bm;
    eb_bm_init(&bm);
    int id = eb_bm_add(&bm, "Example", "https://example.com", 1);
    ASSERT(id > 0);
    eb_bookmark_t *b = eb_bm_get(&bm, id);
    ASSERT(b != NULL);
    ASSERT(strcmp(b->title, "Example") == 0);
    ASSERT(strcmp(b->url, "https://example.com") == 0);
    ASSERT(b->type == EB_BM_BOOKMARK);
    ASSERT(b->parent_id == 1);
    ASSERT(b->created_at > 0);
    eb_bm_destroy(&bm);
}

TEST(test_add_folder) {
    static eb_bookmark_mgr_t bm;
    eb_bm_init(&bm);
    int id = eb_bm_add_folder(&bm, "Work", 1);
    ASSERT(id > 0);
    eb_bookmark_t *f = eb_bm_get(&bm, id);
    ASSERT(f != NULL);
    ASSERT(f->type == EB_BM_FOLDER);
    ASSERT(strcmp(f->title, "Work") == 0);
    eb_bm_destroy(&bm);
}

TEST(test_remove_bookmark) {
    static eb_bookmark_mgr_t bm;
    eb_bm_init(&bm);
    int id = eb_bm_add(&bm, "Temp", "https://temp.com", 1);
    int count_before = bm.count;
    ASSERT(eb_bm_remove(&bm, id) == true);
    ASSERT(bm.count == count_before - 1);
    ASSERT(eb_bm_get(&bm, id) == NULL);
    ASSERT(eb_bm_remove(&bm, id) == false); /* Already removed */
    eb_bm_destroy(&bm);
}

TEST(test_update_bookmark) {
    static eb_bookmark_mgr_t bm;
    eb_bm_init(&bm);
    int id = eb_bm_add(&bm, "Old", "https://old.com", 1);
    ASSERT(eb_bm_update(&bm, id, "New", "https://new.com") == true);
    eb_bookmark_t *b = eb_bm_get(&bm, id);
    ASSERT(strcmp(b->title, "New") == 0);
    ASSERT(strcmp(b->url, "https://new.com") == 0);
    eb_bm_destroy(&bm);
}

TEST(test_exists_url) {
    static eb_bookmark_mgr_t bm;
    eb_bm_init(&bm);
    eb_bm_add(&bm, "GitHub", "https://github.com", 1);
    ASSERT(eb_bm_exists_url(&bm, "https://github.com") == true);
    ASSERT(eb_bm_exists_url(&bm, "https://gitlab.com") == false);
    eb_bm_destroy(&bm);
}

TEST(test_search) {
    static eb_bookmark_mgr_t bm;
    eb_bm_init(&bm);
    eb_bm_add(&bm, "GitHub", "https://github.com", 1);
    eb_bm_add(&bm, "GitLab", "https://gitlab.com", 1);
    eb_bm_add(&bm, "Example", "https://example.com", 1);
    int results[10]; int n;
    n = eb_bm_search(&bm, "git", results, 10);
    ASSERT(n == 2);
    n = eb_bm_search(&bm, "example", results, 10);
    ASSERT(n == 1);
    n = eb_bm_search(&bm, "nonexistent", results, 10);
    ASSERT(n == 0);
    eb_bm_destroy(&bm);
}

TEST(test_get_children) {
    static eb_bookmark_mgr_t bm;
    eb_bm_init(&bm);
    int folder = eb_bm_add_folder(&bm, "Dev", 1);
    eb_bm_add(&bm, "A", "https://a.com", folder);
    eb_bm_add(&bm, "B", "https://b.com", folder);
    eb_bm_add(&bm, "C", "https://c.com", 1); /* Different parent */
    int results[10]; int n;
    n = eb_bm_get_children(&bm, folder, results, 10);
    ASSERT(n == 2);
    eb_bm_destroy(&bm);
}

TEST(test_tags) {
    static eb_bookmark_mgr_t bm;
    eb_bm_init(&bm);
    int id = eb_bm_add(&bm, "Tagged", "https://tagged.com", 1);
    ASSERT(eb_bm_add_tag(&bm, id, "programming") == true);
    ASSERT(eb_bm_add_tag(&bm, id, "favorite") == true);
    eb_bookmark_t *b = eb_bm_get(&bm, id);
    ASSERT(b->tag_count == 2);
    ASSERT(strcmp(b->tags[0], "programming") == 0);
    ASSERT(strcmp(b->tags[1], "favorite") == 0);
    eb_bm_destroy(&bm);
}

TEST(test_move) {
    static eb_bookmark_mgr_t bm;
    eb_bm_init(&bm);
    int f1 = eb_bm_add_folder(&bm, "F1", 0);
    int f2 = eb_bm_add_folder(&bm, "F2", 0);
    int id = eb_bm_add(&bm, "Movable", "https://move.com", f1);
    eb_bookmark_t *b = eb_bm_get(&bm, id);
    ASSERT(b->parent_id == f1);
    ASSERT(eb_bm_move(&bm, id, f2, 0) == true);
    ASSERT(b->parent_id == f2);
    eb_bm_destroy(&bm);
}

TEST(test_get_nonexistent) {
    static eb_bookmark_mgr_t bm;
    eb_bm_init(&bm);
    ASSERT(eb_bm_get(&bm, 9999) == NULL);
    eb_bm_destroy(&bm);
}

/* The browser runs eb_bm_init() and then eb_bm_load() on every start, so a
 * save followed by that sequence must give back the same bookmarks. */
TEST(test_save_load_round_trip) {
    static eb_bookmark_mgr_t bm, loaded;
    const char *path = "test_bookmark_round_trip.txt";
    eb_bm_init(&bm);
    int work = eb_bm_add_folder(&bm, "Work", 1);
    int ex = eb_bm_add(&bm, "Example", "https://example.com/a?b=c", 1);
    int docs = eb_bm_add(&bm, "Docs | Reference", "https://docs.example.org/", work);
    int other = eb_bm_add(&bm, "Other", "https://other.example/", 2);
    ASSERT(eb_bm_save(&bm, path) == bm.count);

    eb_bm_init(&loaded);
    ASSERT(eb_bm_load(&loaded, path) == bm.count);
    remove(path);
    ASSERT(loaded.count == bm.count);   /* no second copy of the root folders */

    eb_bookmark_t *b = eb_bm_get(&loaded, ex);
    ASSERT(b != NULL);
    ASSERT(b->type == EB_BM_BOOKMARK);
    ASSERT(strcmp(b->title, "Example") == 0);
    ASSERT(strcmp(b->url, "https://example.com/a?b=c") == 0);
    ASSERT(b->parent_id == 1);

    b = eb_bm_get(&loaded, docs);
    ASSERT(b != NULL);
    ASSERT(strcmp(b->title, "Docs | Reference") == 0);
    ASSERT(strcmp(b->url, "https://docs.example.org/") == 0);
    ASSERT(b->parent_id == work);

    b = eb_bm_get(&loaded, work);
    ASSERT(b != NULL);
    ASSERT(b->type == EB_BM_FOLDER);
    ASSERT(strcmp(b->title, "Work") == 0);
    ASSERT(b->url[0] == '\0');
    ASSERT(b->parent_id == 1);

    b = eb_bm_get(&loaded, other);
    ASSERT(b != NULL && b->parent_id == 2);
    ASSERT(strcmp(eb_bm_get(&loaded, 1)->title, "Bookmarks Bar") == 0);
    ASSERT(eb_bm_exists_url(&loaded, "https://example.com/a?b=c"));

    int kids[8];
    ASSERT(eb_bm_get_children(&loaded, 1, kids, 8) == 2); /* Work, Example */

    /* New items continue after the highest loaded id. */
    ASSERT(eb_bm_add(&loaded, "New", "https://new.example/", 1) == other + 1);
    eb_bm_destroy(&bm);
    eb_bm_destroy(&loaded);
}

int main(void) {
    printf("=== Bookmark Tests ===\n");
    RUN(test_init); RUN(test_add_bookmark); RUN(test_add_folder);
    RUN(test_remove_bookmark); RUN(test_update_bookmark);
    RUN(test_exists_url); RUN(test_search); RUN(test_get_children);
    RUN(test_tags); RUN(test_move); RUN(test_get_nonexistent);
    RUN(test_save_load_round_trip);
    printf("\nResults: %d passed, %d failed\n", s_pass, s_fail);
    return s_fail > 0 ? 1 : 0;
}
