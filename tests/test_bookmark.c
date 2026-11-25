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

/* --- Malformed-file fixtures (#67): a partial load must never be saved
 * over the original file. Each test writes a fixture, loads it the way
 * the browser does (init, then load), and proves the file survives. --- */

static void write_fixture(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    ASSERT(f != NULL);
    if (!f) return;
    fputs(content, f);
    fclose(f);
}

static bool file_equals(const char *path, const char *expected) {
    FILE *f = fopen(path, "r");
    if (!f) return false;
    char buf[8192];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';
    return strcmp(buf, expected) == 0;
}

TEST(test_load_bad_line_partial) {
    static eb_bookmark_mgr_t bm;
    const char *path = "test_bm_bad_line.txt";
    const char *fixture =
        "eBrowser Bookmarks v1\n"
        "3|0|Good|https://good.com|1\n"
        "this line is not a bookmark\n"
        "4|0|Also Good|https://alsogood.com|1\n";
    write_fixture(path, fixture);
    eb_bm_init(&bm);
    ASSERT(eb_bm_load(&bm, path) == -2);
    ASSERT(bm.load_errors == 1);
    ASSERT(bm.count == 4); /* 2 roots + 2 good records */
    ASSERT(eb_bm_get(&bm, 3) != NULL);
    ASSERT(eb_bm_get(&bm, 4) != NULL);
    ASSERT(bm.next_id == 5);
    /* The partial load must never be persisted over the original. */
    ASSERT(eb_bm_save(&bm, path) == -1);
    ASSERT(file_equals(path, fixture));
    remove(path);
    eb_bm_destroy(&bm);
}

TEST(test_load_truncated) {
    static eb_bookmark_mgr_t bm;
    const char *path = "test_bm_truncated.txt";
    const char *fixture =
        "eBrowser Bookmarks v1\n"
        "3|0|Example|https://example.com|1\n"
        "4|0|Cut|https://cut.examp"; /* no trailing newline: truncated */
    write_fixture(path, fixture);
    eb_bm_init(&bm);
    ASSERT(eb_bm_load(&bm, path) == -2);
    ASSERT(bm.load_errors == 1);
    ASSERT(bm.count == 3); /* 2 roots + the one complete record */
    ASSERT(eb_bm_get(&bm, 3) != NULL);
    ASSERT(eb_bm_get(&bm, 4) == NULL); /* cut record is not loaded */
    ASSERT(eb_bm_save(&bm, path) == -1);
    ASSERT(file_equals(path, fixture));
    remove(path);
    eb_bm_destroy(&bm);
}

TEST(test_load_duplicate_blank_record) {
    /* The #66 hold note: a file corrupted by #65 has the complete id-1
     * line first and a blank id-1 line later. The blank duplicate must
     * never replace the complete record, and the file must survive. */
    static eb_bookmark_mgr_t bm;
    const char *path = "test_bm_duplicate.txt";
    const char *fixture =
        "eBrowser Bookmarks v1\n"
        "1|1|Bookmarks Bar||0\n"
        "1|1|||0\n"
        "3|0|Example|https://example.com|1\n";
    write_fixture(path, fixture);
    eb_bm_init(&bm);
    ASSERT(eb_bm_load(&bm, path) == -2);
    ASSERT(bm.load_errors == 1);
    ASSERT(bm.count == 3); /* 2 roots + the one good record, no duplicates */
    eb_bookmark_t *bar = eb_bm_get(&bm, 1);
    ASSERT(bar != NULL);
    ASSERT(strcmp(bar->title, "Bookmarks Bar") == 0);
    ASSERT(eb_bm_get(&bm, 3) != NULL);
    ASSERT(eb_bm_save(&bm, path) == -1);
    ASSERT(file_equals(path, fixture));
    remove(path);
    eb_bm_destroy(&bm);
}

TEST(test_load_bad_header) {
    static eb_bookmark_mgr_t bm;
    const char *path = "test_bm_bad_header.txt";
    const char *fixture =
        "not a bookmarks file\n"
        "3|0|X|https://x.com|1\n";
    write_fixture(path, fixture);
    eb_bm_init(&bm);
    ASSERT(eb_bm_load(&bm, path) == -2);
    ASSERT(bm.load_errors == 1);
    ASSERT(bm.count == 2); /* init roots only; nothing loaded */
    ASSERT(eb_bm_save(&bm, path) == -1);
    ASSERT(file_equals(path, fixture));
    remove(path);
    eb_bm_destroy(&bm);
}

TEST(test_load_rejects_out_of_range_id) {
    /* INT_MAX would overflow next_id (id + 1); negative ids are invalid. */
    static eb_bookmark_mgr_t bm;
    const char *path = "test_bm_range.txt";
    const char *fixture =
        "eBrowser Bookmarks v1\n"
        "2147483647|0|Big|https://big.com|1\n"
        "-5|0|Neg|https://neg.com|1\n"
        "3|0|Good|https://good.com|1\n";
    write_fixture(path, fixture);
    eb_bm_init(&bm);
    ASSERT(eb_bm_load(&bm, path) == -2);
    ASSERT(bm.load_errors == 2);
    ASSERT(bm.count == 3);
    ASSERT(eb_bm_get(&bm, 3) != NULL);
    ASSERT(bm.next_id == 4);
    ASSERT(eb_bm_save(&bm, path) == -1);
    ASSERT(file_equals(path, fixture));
    remove(path);
    eb_bm_destroy(&bm);
}

TEST(test_save_atomic_with_backup) {
    static eb_bookmark_mgr_t bm;
    static eb_bookmark_mgr_t bm2;
    const char *path = "test_bm_atomic.txt";
    const char *bak = "test_bm_atomic.txt.bak";
    const char *tmp = "test_bm_atomic.txt.tmp";
    const char *first =
        "eBrowser Bookmarks v1\n"
        "1|1|Bookmarks Bar||0\n"
        "2|1|Other Bookmarks||0\n"
        "3|0|One|https://one.com|1\n";
    const char *second =
        "eBrowser Bookmarks v1\n"
        "1|1|Bookmarks Bar||0\n"
        "2|1|Other Bookmarks||0\n"
        "3|0|One|https://one.com|1\n"
        "4|0|Two|https://two.com|1\n";
    remove(path); remove(bak); remove(tmp);
    eb_bm_init(&bm);
    eb_bm_add(&bm, "One", "https://one.com", 1);
    ASSERT(eb_bm_save(&bm, path) == 3);
    ASSERT(file_equals(path, first));
    /* A second save keeps the previous file as the last-good backup. */
    eb_bm_add(&bm, "Two", "https://two.com", 1);
    ASSERT(eb_bm_save(&bm, path) == 4);
    ASSERT(file_equals(path, second));
    ASSERT(file_equals(bak, first));
    /* No temp file is left behind. */
    FILE *leftover = fopen(tmp, "r");
    ASSERT(leftover == NULL);
    /* A clean load may be saved again. */
    eb_bm_init(&bm2);
    ASSERT(eb_bm_load(&bm2, path) == 4);
    ASSERT(bm2.load_errors == 0);
    ASSERT(bm2.count == 4);
    ASSERT(eb_bm_save(&bm2, path) == 4);
    ASSERT(file_equals(bak, second));
    remove(path); remove(bak); remove(tmp);
    eb_bm_destroy(&bm);
    eb_bm_destroy(&bm2);
}

TEST(test_load_over_capacity) {
    /* More records than the manager holds: the extras are skipped, the
     * load is partial, and the file is preserved. */
    static eb_bookmark_mgr_t bm;
    const char *path = "test_bm_capacity.txt";
    FILE *f = fopen(path, "w");
    ASSERT(f != NULL);
    if (!f) return;
    fputs("eBrowser Bookmarks v1\n", f);
    for (int i = 3; i < 3 + EB_BM_MAX_ITEMS + 5; i++)
        fprintf(f, "%d|0|B%d|https://b%d.com|1\n", i, i, i);
    fclose(f);
    eb_bm_init(&bm);
    ASSERT(eb_bm_load(&bm, path) == -2);
    ASSERT(bm.load_errors == 7); /* 1029 lines, room for 1022 after the roots */
    ASSERT(bm.count == EB_BM_MAX_ITEMS);
    ASSERT(eb_bm_save(&bm, path) == -1);
    remove(path);
    eb_bm_destroy(&bm);
}

TEST(test_load_restores_from_backup) {
    /* The main file may be missing after a save died between moving it
     * aside and installing the new copy; the last-good backup still
     * holds the bookmarks. */
    static eb_bookmark_mgr_t bm;
    const char *path = "test_bm_restore.txt";
    const char *bak = "test_bm_restore.txt.bak";
    const char *content =
        "eBrowser Bookmarks v1\n"
        "1|1|Bookmarks Bar||0\n"
        "2|1|Other Bookmarks||0\n"
        "3|0|Kept|https://kept.com|1\n";
    remove(path); remove(bak);
    write_fixture(bak, content); /* only the backup exists: main file lost */
    eb_bm_init(&bm);
    ASSERT(eb_bm_load(&bm, path) == 3);
    ASSERT(bm.load_errors == 0);
    ASSERT(bm.count == 3);
    ASSERT(eb_bm_get(&bm, 3) != NULL);
    /* Saving now recreates the main file. Since #66 the load reads the
     * title, url and parent_id too, so the restored file is identical to
     * the backup it came from. */
    ASSERT(strcmp(eb_bm_get(&bm, 3)->title, "Kept") == 0);
    ASSERT(strcmp(eb_bm_get(&bm, 3)->url, "https://kept.com") == 0);
    ASSERT(eb_bm_get(&bm, 3)->parent_id == 1);
    ASSERT(eb_bm_save(&bm, path) == 3);
    ASSERT(file_equals(path, content));
    remove(path); remove(bak);
    eb_bm_destroy(&bm);
}

int main(void) {
    printf("=== Bookmark Tests ===\n");
    RUN(test_init); RUN(test_add_bookmark); RUN(test_add_folder);
    RUN(test_remove_bookmark); RUN(test_update_bookmark);
    RUN(test_exists_url); RUN(test_search); RUN(test_get_children);
    RUN(test_tags); RUN(test_move); RUN(test_get_nonexistent);
    RUN(test_save_load_round_trip);
    RUN(test_load_bad_line_partial); RUN(test_load_truncated);
    RUN(test_load_duplicate_blank_record); RUN(test_load_bad_header);
    RUN(test_load_rejects_out_of_range_id); RUN(test_save_atomic_with_backup);
    RUN(test_load_restores_from_backup); RUN(test_load_over_capacity);
    printf("\nResults: %d passed, %d failed\n", s_pass, s_fail);
    return s_fail > 0 ? 1 : 0;
}
