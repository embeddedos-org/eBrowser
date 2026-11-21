// SPDX-License-Identifier: MIT
#include "eBrowser/download.h"
#include <stdio.h>
#include <string.h>

static int s_pass = 0, s_fail = 0, s_cur_fail = 0;
#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %s... ", #name); s_cur_fail = 0; name(); \
    if (s_cur_fail) s_fail++; else { printf("PASS\n"); s_pass++; } } while(0)
#define ASSERT(cond) do { if(!(cond)) { printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); s_cur_fail = 1; return; } } while(0)

static const char *name_for(const char *url) {
    static eb_download_mgr_t dm;
    eb_dl_init(&dm, "/dl");
    int id = eb_dl_start(&dm, url, NULL);
    if (id <= 0) return "";
    return eb_dl_get(&dm, id)->filename;
}

static const char *path_for(const char *url) {
    static eb_download_mgr_t dm;
    eb_dl_init(&dm, "/dl");
    int id = eb_dl_start(&dm, url, NULL);
    if (id <= 0) return "";
    return eb_dl_get(&dm, id)->save_path;
}

TEST(test_name_from_last_path_segment) {
    ASSERT(strcmp(name_for("https://example.com/files/report.pdf"), "report.pdf") == 0);
}

TEST(test_name_ignores_query) {
    ASSERT(strcmp(name_for("https://example.com/files/report.pdf?version=2"), "report.pdf") == 0);
    ASSERT(strcmp(path_for("https://example.com/files/report.pdf?version=2"), "/dl/report.pdf") == 0);
}

TEST(test_name_ignores_fragment) {
    ASSERT(strcmp(name_for("https://example.com/files/report.pdf#page=3"), "report.pdf") == 0);
}

TEST(test_slash_in_query_does_not_start_a_segment) {
    ASSERT(strcmp(name_for("https://example.com/get/archive.zip?next=/home/index.html"), "archive.zip") == 0);
}

TEST(test_trailing_slash_falls_back_to_download) {
    ASSERT(strcmp(name_for("https://example.com/files/"), "download") == 0);
    ASSERT(strcmp(name_for("https://example.com/files/?page=2"), "download") == 0);
}

TEST(test_explicit_filename_is_kept) {
    static eb_download_mgr_t dm;
    eb_dl_init(&dm, "/dl");
    int id = eb_dl_start(&dm, "https://example.com/a.bin?x=1", "chosen.bin");
    ASSERT(id > 0);
    ASSERT(strcmp(eb_dl_get(&dm, id)->filename, "chosen.bin") == 0);
}

int main(void) {
    printf("=== Download Tests ===\n");
    RUN(test_name_from_last_path_segment); RUN(test_name_ignores_query);
    RUN(test_name_ignores_fragment); RUN(test_slash_in_query_does_not_start_a_segment);
    RUN(test_trailing_slash_falls_back_to_download); RUN(test_explicit_filename_is_kept);
    printf("\nResults: %d passed, %d failed\n", s_pass, s_fail);
    return s_fail > 0 ? 1 : 0;
}
